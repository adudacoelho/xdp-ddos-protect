#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <bpf/libbpf.h>

#define MAX_STRIKES 5

struct rate_limit_entry {
    unsigned long long last_update;
    unsigned int packet_count;
};

int main()
{
    int rate_fd, blacklist_fd;

    rate_fd = bpf_obj_get("/sys/fs/bpf/rate_limit_map");
    blacklist_fd = bpf_obj_get("/sys/fs/bpf/blacklist_map");

    if (rate_fd < 0 || blacklist_fd < 0) {
        printf("Error opening maps\n");
        return 1;
    }

    __u32 key = 0;
    __u32 next_key;

    int strikes[1024] = {0};

    while (1) {

        while (bpf_map_get_next_key(rate_fd, &key, &next_key) == 0) {

            struct rate_limit_entry value;

            bpf_map_lookup_elem(rate_fd, &next_key, &value);

            if (value.packet_count > 250) {

                strikes[next_key]++;

                if (strikes[next_key] >= MAX_STRIKES) {

                    __u8 blocked = 1;

                    bpf_map_update_elem(
                        blacklist_fd,
                        &next_key,
                        &blocked,
                        BPF_ANY
                    );

                    struct in_addr ip_addr;
                    ip_addr.s_addr = htonl(next_key);

                    printf(
                        "[BLACKLISTED] %s\n",
                        inet_ntoa(ip_addr)
                    );
                }
            }

            key = next_key;
        }

        sleep(1);
    }

    return 0;
}