_PyMemoTable_Lookup(PyMemoTable *self, PyObject *key)
{
    size_t i;
    size_t perturb;
    size_t mask = (size_t)self->mt_mask;
    PyMemoEntry *table = self->mt_table;
    PyMemoEntry *entry;
    Py_hash_t hash = (Py_hash_t)key >> 3;

    i = hash & mask;
    entry = &table[i];
    if (entry->me_key == NULL || entry->me_key == key)
        return entry;

    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;
        entry = &table[i & mask];
        if (entry->me_key == NULL || entry->me_key == key)
            return entry;
    }
    Py_UNREACHABLE();
}