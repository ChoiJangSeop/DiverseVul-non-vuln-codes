CreateStatistics(CreateStatsStmt *stmt)
{
	int16		attnums[STATS_MAX_DIMENSIONS];
	int			nattnums = 0;
	int			numcols;
	char	   *namestr;
	NameData	stxname;
	Oid			statoid;
	Oid			namespaceId;
	Oid			stxowner = GetUserId();
	HeapTuple	htup;
	Datum		values[Natts_pg_statistic_ext];
	bool		nulls[Natts_pg_statistic_ext];
	Datum		datavalues[Natts_pg_statistic_ext_data];
	bool		datanulls[Natts_pg_statistic_ext_data];
	int2vector *stxkeys;
	List	   *stxexprs = NIL;
	Datum		exprsDatum;
	Relation	statrel;
	Relation	datarel;
	Relation	rel = NULL;
	Oid			relid;
	ObjectAddress parentobject,
				myself;
	Datum		types[4];		/* one for each possible type of statistic */
	int			ntypes;
	ArrayType  *stxkind;
	bool		build_ndistinct;
	bool		build_dependencies;
	bool		build_mcv;
	bool		build_expressions;
	bool		requested_type = false;
	int			i;
	ListCell   *cell;
	ListCell   *cell2;

	Assert(IsA(stmt, CreateStatsStmt));

	/*
	 * Examine the FROM clause.  Currently, we only allow it to be a single
	 * simple table, but later we'll probably allow multiple tables and JOIN
	 * syntax.  The grammar is already prepared for that, so we have to check
	 * here that what we got is what we can support.
	 */
	if (list_length(stmt->relations) != 1)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("only a single relation is allowed in CREATE STATISTICS")));

	foreach(cell, stmt->relations)
	{
		Node	   *rln = (Node *) lfirst(cell);

		if (!IsA(rln, RangeVar))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("only a single relation is allowed in CREATE STATISTICS")));

		/*
		 * CREATE STATISTICS will influence future execution plans but does
		 * not interfere with currently executing plans.  So it should be
		 * enough to take only ShareUpdateExclusiveLock on relation,
		 * conflicting with ANALYZE and other DDL that sets statistical
		 * information, but not with normal queries.
		 */
		rel = relation_openrv((RangeVar *) rln, ShareUpdateExclusiveLock);

		/* Restrict to allowed relation types */
		if (rel->rd_rel->relkind != RELKIND_RELATION &&
			rel->rd_rel->relkind != RELKIND_MATVIEW &&
			rel->rd_rel->relkind != RELKIND_FOREIGN_TABLE &&
			rel->rd_rel->relkind != RELKIND_PARTITIONED_TABLE)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("relation \"%s\" is not a table, foreign table, or materialized view",
							RelationGetRelationName(rel))));

		/* You must own the relation to create stats on it */
		if (!pg_class_ownercheck(RelationGetRelid(rel), stxowner))
			aclcheck_error(ACLCHECK_NOT_OWNER, get_relkind_objtype(rel->rd_rel->relkind),
						   RelationGetRelationName(rel));

		/* Creating statistics on system catalogs is not allowed */
		if (!allowSystemTableMods && IsSystemRelation(rel))
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied: \"%s\" is a system catalog",
							RelationGetRelationName(rel))));
	}

	Assert(rel);
	relid = RelationGetRelid(rel);

	/*
	 * If the node has a name, split it up and determine creation namespace.
	 * If not (a possibility not considered by the grammar, but one which can
	 * occur via the "CREATE TABLE ... (LIKE)" command), then we put the
	 * object in the same namespace as the relation, and cons up a name for
	 * it.
	 */
	if (stmt->defnames)
		namespaceId = QualifiedNameGetCreationNamespace(stmt->defnames,
														&namestr);
	else
	{
		namespaceId = RelationGetNamespace(rel);
		namestr = ChooseExtendedStatisticName(RelationGetRelationName(rel),
											  ChooseExtendedStatisticNameAddition(stmt->exprs),
											  "stat",
											  namespaceId);
	}
	namestrcpy_s(&stxname, sizeof(&stxname), namestr);

	/*
	 * Deal with the possibility that the statistics object already exists.
	 */
	if (SearchSysCacheExists2(STATEXTNAMENSP,
							  CStringGetDatum(namestr),
							  ObjectIdGetDatum(namespaceId)))
	{
		if (stmt->if_not_exists)
		{
			ereport(NOTICE,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("statistics object \"%s\" already exists, skipping",
							namestr)));
			relation_close(rel, NoLock);
			return InvalidObjectAddress;
		}

		ereport(ERROR,
				(errcode(ERRCODE_DUPLICATE_OBJECT),
				 errmsg("statistics object \"%s\" already exists", namestr)));
	}

	/*
	 * Make sure no more than STATS_MAX_DIMENSIONS columns are used. There
	 * might be duplicates and so on, but we'll deal with those later.
	 */
	numcols = list_length(stmt->exprs);
	if (numcols > STATS_MAX_DIMENSIONS)
		ereport(ERROR,
				(errcode(ERRCODE_TOO_MANY_COLUMNS),
				 errmsg("cannot have more than %d columns in statistics",
						STATS_MAX_DIMENSIONS)));

	/*
	 * Convert the expression list to a simple array of attnums, but also keep
	 * a list of more complex expressions.  While at it, enforce some
	 * constraints - we don't allow extended statistics on system attributes,
	 * and we require the data type to have less-than operator.
	 *
	 * There are many ways how to "mask" a simple attribute refenrece as an
	 * expression, for example "(a+0)" etc. We can't possibly detect all of
	 * them, but we handle at least the simple case with attribute in parens.
	 * There'll always be a way around this, if the user is determined (like
	 * the "(a+0)" example), but this makes it somewhat consistent with how
	 * indexes treat attributes/expressions.
	 */
	foreach(cell, stmt->exprs)
	{
		StatsElem  *selem = lfirst_node(StatsElem, cell);

		if (selem->name)		/* column reference */
		{
			char	   *attname;
			HeapTuple	atttuple;
			Form_pg_attribute attForm;
			TypeCacheEntry *type;

			attname = selem->name;

			atttuple = SearchSysCacheAttName(relid, attname);
			if (!HeapTupleIsValid(atttuple))
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_COLUMN),
						 errmsg("column \"%s\" does not exist",
								attname)));
			attForm = (Form_pg_attribute) GETSTRUCT(atttuple);

			/* Disallow use of system attributes in extended stats */
			if (attForm->attnum <= 0)
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("statistics creation on system columns is not supported")));

			/* Disallow data types without a less-than operator */
			type = lookup_type_cache(attForm->atttypid, TYPECACHE_LT_OPR);
			if (type->lt_opr == InvalidOid)
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("column \"%s\" cannot be used in statistics because its type %s has no default btree operator class",
								attname, format_type_be(attForm->atttypid))));

			attnums[nattnums] = attForm->attnum;
			nattnums++;
			ReleaseSysCache(atttuple);
		}
		else if (IsA(selem->expr, Var))	/* column reference in parens */
		{
			Var *var = (Var *) selem->expr;
			TypeCacheEntry *type;

			/* Disallow use of system attributes in extended stats */
			if (var->varattno <= 0)
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("statistics creation on system columns is not supported")));

			/* Disallow data types without a less-than operator */
			type = lookup_type_cache(var->vartype, TYPECACHE_LT_OPR);
			if (type->lt_opr == InvalidOid)
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("column \"%s\" cannot be used in statistics because its type %s has no default btree operator class",
								get_attname(relid, var->varattno, false), format_type_be(var->vartype))));

			attnums[nattnums] = var->varattno;
			nattnums++;
		}
		else					/* expression */
		{
			Node	   *expr = selem->expr;
			Oid			atttype;
			TypeCacheEntry *type;
			Bitmapset  *attnums = NULL;
			int			k;

			Assert(expr != NULL);

			/* Disallow expressions referencing system attributes. */
			pull_varattnos(expr, 1, &attnums);

			k = -1;
			while ((k = bms_next_member(attnums, k)) >= 0)
			{
				AttrNumber	attnum = k + FirstLowInvalidHeapAttributeNumber;
				if (attnum <= 0)
					ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("statistics creation on system columns is not supported")));
			}

			/*
			 * Disallow data types without a less-than operator.
			 *
			 * We ignore this for statistics on a single expression, in which
			 * case we'll build the regular statistics only (and that code can
			 * deal with such data types).
			 */
			if (list_length(stmt->exprs) > 1)
			{
				atttype = exprType(expr);
				type = lookup_type_cache(atttype, TYPECACHE_LT_OPR);
				if (type->lt_opr == InvalidOid)
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("expression cannot be used in multivariate statistics because its type %s has no default btree operator class",
									format_type_be(atttype))));
			}

			stxexprs = lappend(stxexprs, expr);
		}
	}

	/*
	 * Parse the statistics kinds.
	 *
	 * First check that if this is the case with a single expression, there
	 * are no statistics kinds specified (we don't allow that for the simple
	 * CREATE STATISTICS form).
	 */
	if ((list_length(stmt->exprs) == 1) && (list_length(stxexprs) == 1))
	{
		/* statistics kinds not specified */
		if (list_length(stmt->stat_types) > 0)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("when building statistics on a single expression, statistics kinds may not be specified")));
	}

	/* OK, let's check that we recognize the statistics kinds. */
	build_ndistinct = false;
	build_dependencies = false;
	build_mcv = false;
	foreach(cell, stmt->stat_types)
	{
		char	   *type = strVal((Value *) lfirst(cell));

		if (strcmp(type, "ndistinct") == 0)
		{
			build_ndistinct = true;
			requested_type = true;
		}
		else if (strcmp(type, "dependencies") == 0)
		{
			build_dependencies = true;
			requested_type = true;
		}
		else if (strcmp(type, "mcv") == 0)
		{
			build_mcv = true;
			requested_type = true;
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("unrecognized statistics kind \"%s\"",
							type)));
	}

	/*
	 * If no statistic type was specified, build them all (but only when the
	 * statistics is defined on more than one column/expression).
	 */
	if ((!requested_type) && (numcols >= 2))
	{
		build_ndistinct = true;
		build_dependencies = true;
		build_mcv = true;
	}

	/*
	 * When there are non-trivial expressions, build the expression stats
	 * automatically. This allows calculating good estimates for stats that
	 * consider per-clause estimates (e.g. functional dependencies).
	 */
	build_expressions = (list_length(stxexprs) > 0);

	/*
	 * Check that at least two columns were specified in the statement, or
	 * that we're building statistics on a single expression.
	 */
	if ((numcols < 2) && (list_length(stxexprs) != 1))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
				 errmsg("extended statistics require at least 2 columns")));

	/*
	 * Sort the attnums, which makes detecting duplicates somewhat easier, and
	 * it does not hurt (it does not matter for the contents, unlike for
	 * indexes, for example).
	 */
	qsort(attnums, nattnums, sizeof(int16), compare_int16);

	/*
	 * Check for duplicates in the list of columns. The attnums are sorted so
	 * just check consecutive elements.
	 */
	for (i = 1; i < nattnums; i++)
	{
		if (attnums[i] == attnums[i - 1])
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_COLUMN),
					 errmsg("duplicate column name in statistics definition")));
	}

	/*
	 * Check for duplicate expressions. We do two loops, counting the
	 * occurrences of each expression. This is O(N^2) but we only allow small
	 * number of expressions and it's not executed often.
	 *
	 * XXX We don't cross-check attributes and expressions, because it does
	 * not seem worth it. In principle we could check that expressions don't
	 * contain trivial attribute references like "(a)", but the reasoning is
	 * similar to why we don't bother with extracting columns from
	 * expressions. It's either expensive or very easy to defeat for
	 * determined user, and there's no risk if we allow such statistics (the
	 * statistics is useless, but harmless).
	 */
	foreach(cell, stxexprs)
	{
		Node	   *expr1 = (Node *) lfirst(cell);
		int			cnt = 0;

		foreach(cell2, stxexprs)
		{
			Node	   *expr2 = (Node *) lfirst(cell2);

			if (equal(expr1, expr2))
				cnt += 1;
		}

		/* every expression should find at least itself */
		Assert(cnt >= 1);

		if (cnt > 1)
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_COLUMN),
					 errmsg("duplicate expression in statistics definition")));
	}

	/* Form an int2vector representation of the sorted column list */
	stxkeys = buildint2vector(attnums, nattnums);

	/* construct the char array of enabled statistic types */
	ntypes = 0;
	if (build_ndistinct)
		types[ntypes++] = CharGetDatum(STATS_EXT_NDISTINCT);
	if (build_dependencies)
		types[ntypes++] = CharGetDatum(STATS_EXT_DEPENDENCIES);
	if (build_mcv)
		types[ntypes++] = CharGetDatum(STATS_EXT_MCV);
	if (build_expressions)
		types[ntypes++] = CharGetDatum(STATS_EXT_EXPRESSIONS);
	Assert(ntypes > 0 && ntypes <= lengthof(types));
	stxkind = construct_array(types, ntypes, CHAROID, 1, true, TYPALIGN_CHAR);

	/* convert the expressions (if any) to a text datum */
	if (stxexprs != NIL)
	{
		char	   *exprsString;

		exprsString = nodeToString(stxexprs);
		exprsDatum = CStringGetTextDatum(exprsString);
		pfree(exprsString);
	}
	else
		exprsDatum = (Datum) 0;

	statrel = table_open(StatisticExtRelationId, RowExclusiveLock);

	/*
	 * Everything seems fine, so let's build the pg_statistic_ext tuple.
	 */
	memset(values, 0, sizeof(values));
	memset(nulls, false, sizeof(nulls));

	statoid = GetNewOidWithIndex(statrel, StatisticExtOidIndexId,
								 Anum_pg_statistic_ext_oid);
	values[Anum_pg_statistic_ext_oid - 1] = ObjectIdGetDatum(statoid);
	values[Anum_pg_statistic_ext_stxrelid - 1] = ObjectIdGetDatum(relid);
	values[Anum_pg_statistic_ext_stxname - 1] = NameGetDatum(&stxname);
	values[Anum_pg_statistic_ext_stxnamespace - 1] = ObjectIdGetDatum(namespaceId);
	values[Anum_pg_statistic_ext_stxstattarget - 1] = Int32GetDatum(-1);
	values[Anum_pg_statistic_ext_stxowner - 1] = ObjectIdGetDatum(stxowner);
	values[Anum_pg_statistic_ext_stxkeys - 1] = PointerGetDatum(stxkeys);
	values[Anum_pg_statistic_ext_stxkind - 1] = PointerGetDatum(stxkind);

	values[Anum_pg_statistic_ext_stxexprs - 1] = exprsDatum;
	if (exprsDatum == (Datum) 0)
		nulls[Anum_pg_statistic_ext_stxexprs - 1] = true;

	/* insert it into pg_statistic_ext */
	htup = heap_form_tuple(statrel->rd_att, values, nulls);
	CatalogTupleInsert(statrel, htup);
	heap_freetuple(htup);

	relation_close(statrel, RowExclusiveLock);

	/*
	 * Also build the pg_statistic_ext_data tuple, to hold the actual
	 * statistics data.
	 */
	datarel = table_open(StatisticExtDataRelationId, RowExclusiveLock);

	memset(datavalues, 0, sizeof(datavalues));
	memset(datanulls, false, sizeof(datanulls));

	datavalues[Anum_pg_statistic_ext_data_stxoid - 1] = ObjectIdGetDatum(statoid);

	/* no statistics built yet */
	datanulls[Anum_pg_statistic_ext_data_stxdndistinct - 1] = true;
	datanulls[Anum_pg_statistic_ext_data_stxddependencies - 1] = true;
	datanulls[Anum_pg_statistic_ext_data_stxdmcv - 1] = true;
	datanulls[Anum_pg_statistic_ext_data_stxdexpr - 1] = true;

	/* insert it into pg_statistic_ext_data */
	htup = heap_form_tuple(datarel->rd_att, datavalues, datanulls);
	CatalogTupleInsert(datarel, htup);
	heap_freetuple(htup);

	relation_close(datarel, RowExclusiveLock);

	InvokeObjectPostCreateHook(StatisticExtRelationId, statoid, 0);

	/*
	 * Invalidate relcache so that others see the new statistics object.
	 */
	CacheInvalidateRelcache(rel);

	relation_close(rel, NoLock);

	/*
	 * Add an AUTO dependency on each column used in the stats, so that the
	 * stats object goes away if any or all of them get dropped.
	 */
	ObjectAddressSet(myself, StatisticExtRelationId, statoid);

	/* add dependencies for plain column references */
	for (i = 0; i < nattnums; i++)
	{
		ObjectAddressSubSet(parentobject, RelationRelationId, relid, attnums[i]);
		recordDependencyOn(&myself, &parentobject, DEPENDENCY_AUTO);
	}

	/*
	 * If there are no dependencies on a column, give the statistics object an
	 * auto dependency on the whole table.  In most cases, this will be
	 * redundant, but it might not be if the statistics expressions contain no
	 * Vars (which might seem strange but possible). This is consistent with
	 * what we do for indexes in index_create.
	 *
	 * XXX We intentionally don't consider the expressions before adding this
	 * dependency, because recordDependencyOnSingleRelExpr may not create any
	 * dependencies for whole-row Vars.
	 */
	if (!nattnums)
	{
		ObjectAddressSet(parentobject, RelationRelationId, relid);
		recordDependencyOn(&myself, &parentobject, DEPENDENCY_AUTO);
	}

	/*
	 * Store dependencies on anything mentioned in statistics expressions,
	 * just like we do for index expressions.
	 */
	if (stxexprs)
		recordDependencyOnSingleRelExpr(&myself,
										(Node *) stxexprs,
										relid,
										DEPENDENCY_NORMAL,
										DEPENDENCY_AUTO, false);

	/*
	 * Also add dependencies on namespace and owner.  These are required
	 * because the stats object might have a different namespace and/or owner
	 * than the underlying table(s).
	 */
	ObjectAddressSet(parentobject, NamespaceRelationId, namespaceId);
	recordDependencyOn(&myself, &parentobject, DEPENDENCY_NORMAL);

	recordDependencyOnOwner(StatisticExtRelationId, statoid, stxowner);

	/*
	 * XXX probably there should be a recordDependencyOnCurrentExtension call
	 * here too, but we'd have to add support for ALTER EXTENSION ADD/DROP
	 * STATISTICS, which is more work than it seems worth.
	 */

	/* Add any requested comment */
	if (stmt->stxcomment != NULL)
		CreateComments(statoid, StatisticExtRelationId, 0,
					   stmt->stxcomment);

	/* Return stats object's address */
	return myself;
}