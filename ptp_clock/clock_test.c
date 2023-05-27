#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define CLOCKFD 3
#define FD_TO_CLOCKID(fd)  ((clockid_t)((((unsigned int) ~fd) << 3) | CLOCKFD))

int main()
{
	clockid_t clkid;
	int fd = 0;
	int rc = 0;
	char *ptp_clock = "/dev/ptp0";
	struct timespec ts;
	char response[128];

	fd = open(ptp_clock, O_RDWR);
	if (fd < 0) {
	    printf("Could not open clock %s, errno:%d\n", ptp_clock, errno);
		return -1;
	}
	clkid = FD_TO_CLOCKID(fd);

	while (1) {

		printf("Option: gettime/settime/exit : ");
		scanf("%s", response);

		if (!strcmp(response, "gettime")) {
			rc = clock_gettime(clkid, &ts);
			if (rc) {
	    		printf("Could not gettime. rc:%d\n", rc);
				continue;
			}
			printf("Got time: %ld . %ld\n", ts.tv_sec, ts.tv_nsec);
		} else if (!strcmp(response, "settime")) {
		    ts.tv_sec = 10;
			ts.tv_nsec = 100;
			rc = clock_settime(clkid, &ts);
			if (rc) {
				printf("Could not settime. rc:%d\n", rc);
				continue;
			}
			printf("Set-time done\n");
		} else if (!strcmp(response, "exit")) {
			printf("Exiting..\n");
		    break;
		} else {
			printf("Invalid option\n");
		}
	}

    
	return 0;
}
