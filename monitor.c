#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

#define MAX_STRIKES 5
#define MAX_IPS 1024
#define RATE_LIMIT 3

struct rate_limit_entry {
    unsigned long long last_update;
    unsigned int packet_count;
};

int main()
{
    int rate_fd;
    int blacklist_fd;

    rate_fd = bpf_obj_get("/sys/fs/bpf/rate_limit_map");

    if (rate_fd < 0) {
        perror("Erro ao abrir rate_limit_map");
        return 1;
    }

    blacklist_fd = bpf_obj_get("/sys/fs/bpf/blacklist_map");

    if (blacklist_fd < 0) {
        perror("Erro ao abrir blacklist_map");
        return 1;
    }

    printf("Mapas abertos com sucesso!\n");


 __u32 key;
    __u32 next_key;

    int strikes[MAX_IPS] = {0};

    while (1) {

        int result;

        result = bpf_map_get_next_key(
            rate_fd,
            NULL,
            &next_key
        );

        while (result == 0) {

            struct rate_limit_entry value;

            if (
                bpf_map_lookup_elem(
                    rate_fd,
                    &next_key,
                    &value
                ) == 0
            ) {

                struct in_addr ip_addr;
                ip_addr.s_addr = next_key;

                printf(
                    "PING de %s -> %u pacotes\n",
                    inet_ntoa(ip_addr),
                    value.packet_count
                );

                if (value.packet_count > RATE_LIMIT) {

                    int index = next_key % MAX_IPS;

                    strikes[index]++;

                    printf(
                        "[STRIKE %d] %s\n",
                        strikes[index],
                        inet_ntoa(ip_addr)
                    );

                    if (strikes[index] >= MAX_STRIKES) {

                        __u8 blocked = 1;

                        if (
                            bpf_map_update_elem(
                                blacklist_fd,
                                &next_key,
                                &blocked,
                                BPF_ANY
                            ) == 0
                        ) {

                            printf(
                                "[BLACKLISTED] %s\n",
                                inet_ntoa(ip_addr)
                            );

                        } else {

                            perror(
                                "Erro ao adicionar IP na blacklist"
                            );
                        }
                    }

                }
            }

            key = next_key;

            result = bpf_map_get_next_key(
                rate_fd,
                &key,
                &next_key
            );
        }

        sleep(1);
    }

    return 0;
}