_Unpickler_ResizeMemoList(UnpicklerObject *self, Py_ssize_t new_size)
{
    Py_ssize_t i;

    assert(new_size > self->memo_size);

    PyObject **memo_new = self->memo;
    PyMem_RESIZE(memo_new, PyObject *, new_size);
    if (memo_new == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    self->memo = memo_new;
    for (i = self->memo_size; i < new_size; i++)
        self->memo[i] = NULL;
    self->memo_size = new_size;
    return 0;
}