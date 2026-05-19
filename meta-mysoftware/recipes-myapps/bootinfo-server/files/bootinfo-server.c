/*
 * bootinfo-server.c — lightweight HTTP telemetry for embedded Linux.
 * GET /bootinfo → JSON system diagnostics (port 8000).
 */

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#define PORT     8000
#define BACKLOG  5
#define BUF_SIZE 4096

static void get_kernel_version(char *buf, size_t len)
{
	struct utsname u;

	if (uname(&u) == 0)
		snprintf(buf, len, "%s", u.release);
	else
		snprintf(buf, len, "unknown");
}

static void get_hostname_str(char *buf, size_t len)
{
	if (gethostname(buf, len) != 0)
		snprintf(buf, len, "unknown");
}

static void get_uptime_human(long seconds, char *buf, size_t len)
{
	long h = seconds / 3600;
	long m = (seconds % 3600) / 60;
	long s = seconds % 60;

	snprintf(buf, len, "%ldh %ldm %lds", h, m, s);
}

static void get_iso_timestamp(char *buf, size_t len)
{
	time_t now = time(NULL);
	struct tm *gmt = gmtime(&now);

	if (gmt)
		strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", gmt);
	else
		snprintf(buf, len, "unknown");
}

static int build_json(char *out, size_t max_len)
{
	struct sysinfo si;
	char kernel[128];
	char hostname[64];
	char uptime_human[32];
	char timestamp[32];

	get_kernel_version(kernel, sizeof(kernel));
	get_hostname_str(hostname, sizeof(hostname));
	get_iso_timestamp(timestamp, sizeof(timestamp));

	if (sysinfo(&si) != 0) {
		snprintf(out, max_len, "{\"error\":\"sysinfo failed\"}");
		return -1;
	}

	long mem_total_mb = (long)(si.totalram * si.mem_unit) / (1024 * 1024);
	long mem_free_mb = (long)(si.freeram * si.mem_unit) / (1024 * 1024);
	double load_avg = (double)si.loads[0] / 65536.0;

	get_uptime_human(si.uptime, uptime_human, sizeof(uptime_human));

	return snprintf(out, max_len,
		"{\n"
		"  \"kernel_version\": \"%s\",\n"
		"  \"uptime_seconds\": %ld,\n"
		"  \"uptime_human\": \"%s\",\n"
		"  \"memory_total_mb\": %ld,\n"
		"  \"memory_free_mb\": %ld,\n"
		"  \"load_avg_1m\": %.2f,\n"
		"  \"hostname\": \"%s\",\n"
		"  \"timestamp\": \"%s\"\n"
		"}\n",
		kernel, si.uptime, uptime_human, mem_total_mb, mem_free_mb,
		load_avg, hostname, timestamp);
}

static void handle_client(int client_fd)
{
	char req_buf[1024];
	char json_body[BUF_SIZE];
	char response[BUF_SIZE + 512];
	ssize_t n;
	int json_len;
	int resp_len;
	const char *not_found =
		"HTTP/1.1 404 Not Found\r\n"
		"Content-Type: text/plain\r\n"
		"Connection: close\r\n\r\n"
		"Not Found. Try GET /bootinfo\n";

	n = read(client_fd, req_buf, sizeof(req_buf) - 1);
	if (n <= 0) {
		close(client_fd);
		return;
	}
	req_buf[n] = '\0';

	if (strncmp(req_buf, "GET /bootinfo", 13) != 0) {
		(void)write(client_fd, not_found, strlen(not_found));
		close(client_fd);
		return;
	}

	json_len = build_json(json_body, sizeof(json_body));
	resp_len = snprintf(response, sizeof(response),
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n"
		"\r\n"
		"%s",
		json_len, json_body);

	(void)write(client_fd, response, (size_t)resp_len);
	close(client_fd);
}

int main(void)
{
	int server_fd;
	int client_fd;
	int opt = 1;
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		close(server_fd);
		return EXIT_FAILURE;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(server_fd);
		return EXIT_FAILURE;
	}

	if (listen(server_fd, BACKLOG) < 0) {
		perror("listen");
		close(server_fd);
		return EXIT_FAILURE;
	}

	printf("bootinfo-server listening on port %d\n", PORT);
	fflush(stdout);

	for (;;) {
		client_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
		if (client_fd < 0) {
			if (errno == EINTR)
				continue;
			perror("accept");
			break;
		}
		handle_client(client_fd);
	}

	close(server_fd);
	return EXIT_SUCCESS;
}
