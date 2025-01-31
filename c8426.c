static bool __io_worker_unuse(struct io_wqe *wqe, struct io_worker *worker)
{
	bool dropped_lock = false;

	if (current->files != worker->restore_files) {
		__acquire(&wqe->lock);
		spin_unlock_irq(&wqe->lock);
		dropped_lock = true;

		task_lock(current);
		current->files = worker->restore_files;
		task_unlock(current);
	}

	/*
	 * If we have an active mm, we need to drop the wq lock before unusing
	 * it. If we do, return true and let the caller retry the idle loop.
	 */
	if (worker->mm) {
		if (!dropped_lock) {
			__acquire(&wqe->lock);
			spin_unlock_irq(&wqe->lock);
			dropped_lock = true;
		}
		__set_current_state(TASK_RUNNING);
		set_fs(KERNEL_DS);
		unuse_mm(worker->mm);
		mmput(worker->mm);
		worker->mm = NULL;
	}

	return dropped_lock;
}