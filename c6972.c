Unpickler_set_memo(UnpicklerObject *self, PyObject *obj)
{
    PyObject **new_memo;
    Py_ssize_t new_memo_size = 0;
    Py_ssize_t i;

    if (obj == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "attribute deletion is not supported");
        return -1;
    }

    if (Py_TYPE(obj) == &UnpicklerMemoProxyType) {
        UnpicklerObject *unpickler =
            ((UnpicklerMemoProxyObject *)obj)->unpickler;

        new_memo_size = unpickler->memo_size;
        new_memo = _Unpickler_NewMemo(new_memo_size);
        if (new_memo == NULL)
            return -1;

        for (i = 0; i < new_memo_size; i++) {
            Py_XINCREF(unpickler->memo[i]);
            new_memo[i] = unpickler->memo[i];
        }
    }
    else if (PyDict_Check(obj)) {
        Py_ssize_t i = 0;
        PyObject *key, *value;

        new_memo_size = PyDict_GET_SIZE(obj);
        new_memo = _Unpickler_NewMemo(new_memo_size);
        if (new_memo == NULL)
            return -1;

        while (PyDict_Next(obj, &i, &key, &value)) {
            Py_ssize_t idx;
            if (!PyLong_Check(key)) {
                PyErr_SetString(PyExc_TypeError,
                                "memo key must be integers");
                goto error;
            }
            idx = PyLong_AsSsize_t(key);
            if (idx == -1 && PyErr_Occurred())
                goto error;
            if (idx < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "memo key must be positive integers.");
                goto error;
            }
            if (_Unpickler_MemoPut(self, idx, value) < 0)
                goto error;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "'memo' attribute must be an UnpicklerMemoProxy object"
                     "or dict, not %.200s", Py_TYPE(obj)->tp_name);
        return -1;
    }

    _Unpickler_MemoCleanup(self);
    self->memo_size = new_memo_size;
    self->memo = new_memo;

    return 0;

  error:
    if (new_memo_size) {
        i = new_memo_size;
        while (--i >= 0) {
            Py_XDECREF(new_memo[i]);
        }
        PyMem_FREE(new_memo);
    }
    return -1;
}