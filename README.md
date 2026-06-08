# Proteção DDoS com XDP (eBPF)

Projeto simples de proteção contra ataques DDoS usando XDP (eBPF). Ele contém um programa BPF (`xdp_ddos_protection.c`) que aplica um rate limit por IP e um monitor em espaço de usuário (`monitor.c`) para inspecionar mapas BPF e adicionar IPs à blacklist quando necessário.

**Principais recursos**
- Limitação de taxa por IP via mapa BPF (`rate_limit_map`).
- Mapa de blacklist (`blacklist_map`) para bloquear IPs que excedem o limite.
- Utilitário `monitor.c` para observar os mapas e mover IPs para a blacklist após múltiplos "strikes".

## Arquivos

- `Makefile`: comandos para compilar, anexar e remover o programa XDP.
- `xdp_ddos_protection.c`: programa eBPF (XDP) que aplica rate limiting a pacotes ICMP (ping).
- `monitor.c`: ferramenta em espaço de usuário para ler os mapas BPF (pinados em `/sys/fs/bpf/`) e adicionar IPs à blacklist.
- `static/`: ativos (ex.: gif de demonstração, imagens).

## Requisitos

- Linux com suporte a eBPF/XDP (kernel >= 5.15 recomendado).
- Ferramentas de compilação: `clang`, `llvm`, `make`.
- Biblioteca e ferramentas BPF: `libbpf-dev`, `bpftool`.
- Utilitários opcionais: `hping3` (testes), `docker` (para rodar alvo de teste).

Instale em distribuições Debian/Ubuntu:

```sh
sudo apt update
sudo apt install clang llvm libbpf-dev bpftool make gcc pkg-config -y
```

## Compilar o programa BPF

O `Makefile` do projeto já define o alvo de compilação. Para compilar o objeto do programa BPF, rode:

```sh
make compile
```

Isto gerará `xdp_ddos_protection.o` (arquivo objeto BPF) conforme definido no `Makefile`.

## Anexar / Remover o programa XDP

Anexar o programa à interface `lo` (padrão):

```sh
make attach
```

Para anexar a outra interface, passe a variável `IFACE`:

```sh
make attach IFACE=eth0
```

Remover o programa da interface:

```sh
make detach
```

## Monitor (espaço de usuário)

O `monitor.c` lê os mapas BPF pinados (ex.: `/sys/fs/bpf/rate_limit_map` e `/sys/fs/bpf/blacklist_map`) e aplica uma política simples de "strikes" para colocar IPs na blacklist.

Compilar o monitor (requer `libbpf` instalado):

```sh
gcc -O2 -o monitor monitor.c $(pkg-config --cflags --libs libbpf)
```

Se ocorrerem erros, instale as dependências de desenvolvimento (`libbpf-dev`, `libelf-dev`, `pkg-config`) e ajuste paths conforme necessário.

Executar o monitor (pode requerer `sudo` para acessar `/sys/fs/bpf`):

```sh
sudo ./monitor
```

## Teste rápido

Rode um serviço alvo (ex.: nginx) localmente com Docker:

```sh
docker run -p 1234:80 nginx
```

Instale `hping3` e dispare pacotes de teste (SYN ou ICMP conforme o seu caso):

```sh
sudo apt install hping3
sudo hping3 -i u1000 -S -p 1234 127.0.0.1
```

No repositório original este projeto aplica rate limiting a pacotes ICMP (ping). Para simular uma carga de ICMP use `hping3 --icmp` ou `ping` com intervals curtos.

## Problemas conhecidos e dicas

- Se a tentativa de anexar o XDP falhar com erro relacionado a LRO (Large Receive Offload), desative LRO na interface alvo:

```sh
sudo ethtool -K eth0 lro off
```

- Se faltar includes ao compilar o BPF, ajuste `C_INCLUDE_PATH` ou instale os headers apropriados (por exemplo `/usr/include/x86_64-linux-gnu`).

## Segurança e permissões

Operações com XDP/eBPF frequentemente exigem permissões de root. Use `sudo` quando necessário e verifique as políticas da máquina antes de bloquear tráfego em produção.

## Licença

Projeto licenciado sob MIT. Veja o arquivo `LICENSE` para mais detalhes.