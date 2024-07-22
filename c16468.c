void bn_sqr_comba8(BN_ULONG *r, const BN_ULONG *a)
	{
	BN_ULONG t1,t2;
	BN_ULONG c1,c2,c3;

	c1=0;
	c2=0;
	c3=0;
	sqr_add_c(a,0,c1,c2,c3);
	r[0]=c1;
	c1=0;
	sqr_add_c2(a,1,0,c2,c3,c1);
	r[1]=c2;
	c2=0;
	sqr_add_c(a,1,c3,c1,c2);
	sqr_add_c2(a,2,0,c3,c1,c2);
	r[2]=c3;
	c3=0;
	sqr_add_c2(a,3,0,c1,c2,c3);
	sqr_add_c2(a,2,1,c1,c2,c3);
	r[3]=c1;
	c1=0;
	sqr_add_c(a,2,c2,c3,c1);
	sqr_add_c2(a,3,1,c2,c3,c1);
	sqr_add_c2(a,4,0,c2,c3,c1);
	r[4]=c2;
	c2=0;
	sqr_add_c2(a,5,0,c3,c1,c2);
	sqr_add_c2(a,4,1,c3,c1,c2);
	sqr_add_c2(a,3,2,c3,c1,c2);
	r[5]=c3;
	c3=0;
	sqr_add_c(a,3,c1,c2,c3);
	sqr_add_c2(a,4,2,c1,c2,c3);
	sqr_add_c2(a,5,1,c1,c2,c3);
	sqr_add_c2(a,6,0,c1,c2,c3);
	r[6]=c1;
	c1=0;
	sqr_add_c2(a,7,0,c2,c3,c1);
	sqr_add_c2(a,6,1,c2,c3,c1);
	sqr_add_c2(a,5,2,c2,c3,c1);
	sqr_add_c2(a,4,3,c2,c3,c1);
	r[7]=c2;
	c2=0;
	sqr_add_c(a,4,c3,c1,c2);
	sqr_add_c2(a,5,3,c3,c1,c2);
	sqr_add_c2(a,6,2,c3,c1,c2);
	sqr_add_c2(a,7,1,c3,c1,c2);
	r[8]=c3;
	c3=0;
	sqr_add_c2(a,7,2,c1,c2,c3);
	sqr_add_c2(a,6,3,c1,c2,c3);
	sqr_add_c2(a,5,4,c1,c2,c3);
	r[9]=c1;
	c1=0;
	sqr_add_c(a,5,c2,c3,c1);
	sqr_add_c2(a,6,4,c2,c3,c1);
	sqr_add_c2(a,7,3,c2,c3,c1);
	r[10]=c2;
	c2=0;
	sqr_add_c2(a,7,4,c3,c1,c2);
	sqr_add_c2(a,6,5,c3,c1,c2);
	r[11]=c3;
	c3=0;
	sqr_add_c(a,6,c1,c2,c3);
	sqr_add_c2(a,7,5,c1,c2,c3);
	r[12]=c1;
	c1=0;
	sqr_add_c2(a,7,6,c2,c3,c1);
	r[13]=c2;
	c2=0;
	sqr_add_c(a,7,c3,c1,c2);
	r[14]=c3;
	r[15]=c1;
	}