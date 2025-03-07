#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <liburing.h>
#include <string.h>

#define FILENAME "test.txt"
#define BUFFER_SIZE 128

int main(int argc, char const *argv[])
{
	struct io_uring ring;
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;
	int fd;
	char buffer[BUFFER_SIZE] = {0};

	printf("io_uring-msg_ring poc\n");

	/* open a file descriptor */;
	fd = open(FILENAME, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	/* initialise io_uring */
	if (io_uring_queue_init(8, &ring, 0) < 0) {
		perror("io_uring_queue_init");
		close(fd);
		return 1;
	}

	/* get an sqe (submission queue entry) */
	sqe = io_uring_get_sqe(&ring);
	if (!sqe) {
		fprintf(stderr, "Failed to get SQE\n");
		io_uring_queue_exit(&ring);
		close(fd);
		return 1;
	}

	/* register the file as a fixed file? */
	io_uring_prep_fixed_fd_install(sqe, fd, 0);

	/* prepare a read request */
	io_uring_prep_read(sqe, fd, buffer, BUFFER_SIZE - 1, 0);

	/* submit request */
	if (io_uring_submit(&ring) < 0) {
		perror("io_uring_submit");
		io_uring_queue_exit(&ring);
		close(fd);
		return 1;
	}

	/* wait for completion */
	if (io_uring_wait_cqe(&ring, &cqe) < 0) {
		perror("io_uring_wait_cqe");
		io_uring_queue_exit(&ring);
		close(fd);
		return 1;
	}

	/* check for errors */
	if (cqe->res < 0) {
		fprintf(stderr, "Async read failed: %s\n", strerror(-cqe->res));
	} else {
		buffer[cqe->res] = '\0'; // null terminate
		printf("\nRead data: %s\n", buffer);
	}

	/* mark cqe as seen */
	io_uring_cqe_seen(&ring, cqe);

	/* cleanup */
	io_uring_queue_exit(&ring);
	close(fd);

	return 0;
}