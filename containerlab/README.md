# 🛡️ XDP DDoS Protection — Containerlab

Proteção contra DDoS usando eBPF/XDP rodando em topologia simulada com Containerlab.

---

## 📁 Estrutura esperada

```
~/clab-xdp/
├── Dockerfile
├── lab.yml
└── xdp-projeto/
    └── xdp-ddos-protect-main/
        ├── Makefile
        ├── xdp_ddos_protection.c
        └── monitor.c
```

---

## ⚙️ Pré-requisitos (já instalados na VM)

- Docker
- Containerlab
- bpftool (`linux-tools-generic`)

---

## 🚀 Como subir o lab do zero (após ligar a VM)

### Passo 1 — Build da imagem da vítima

> Só é necessário fazer isso **uma vez**. Se a imagem já existe, pule para o Passo 2.

```bash
cd ~/clab-xdp
docker build -t xdp-vitima .
```

Para verificar se a imagem já existe:

```bash
docker images | grep xdp-vitima
```

---

### Passo 2 — Subir o lab

```bash
sudo containerlab deploy -t ~/clab-xdp/lab.yml
```

Resultado esperado: tabela com 5 containers (`vitima` + `atacante1` a `atacante4`) com status `running`.

---

### Passo 3 — Configurar IPs nas interfaces de ataque

> Necessário **toda vez** que o lab for recriado, pois os containers não persistem configurações.

**No terminal da VM**, rode:

```bash
# Vítima
sudo docker exec -it clab-xdp-ddos-vitima bash -c "
ip addr add 10.0.1.1/24 dev eth1
ip addr add 10.0.2.1/24 dev eth2
ip addr add 10.0.3.1/24 dev eth3
ip addr add 10.0.4.1/24 dev eth4
"

# Atacantes
sudo docker exec clab-xdp-ddos-atacante1 sh -c "ip addr add 10.0.1.2/24 dev eth1"
sudo docker exec clab-xdp-ddos-atacante2 sh -c "ip addr add 10.0.2.2/24 dev eth2"
sudo docker exec clab-xdp-ddos-atacante3 sh -c "ip addr add 10.0.3.2/24 dev eth3"
sudo docker exec clab-xdp-ddos-atacante4 sh -c "ip addr add 10.0.4.2/24 dev eth4"
```

> ⚠️ Se der erro `ip: not found`, instale o iproute2 nos atacantes primeiro:
> ```bash
> sudo docker exec clab-xdp-ddos-atacante1 sh -c "apt-get update -q && apt-get install -yq iproute2 hping3"
> # Repita para atacante2, atacante3, atacante4
> ```

---

### Passo 4 — Anexar o XDP na vítima

**Abra o Terminal 1** e entre na vítima:

```bash
sudo docker exec -it clab-xdp-ddos-vitima bash
```

Dentro da vítima, reduza o MTU e anexe o XDP em todas as interfaces:

```bash
ip link set dev eth1 mtu 1500
ip link set dev eth2 mtu 1500
ip link set dev eth3 mtu 1500
ip link set dev eth4 mtu 1500

ip link set dev eth1 xdp obj xdp_ddos_protection.o sec xdp
ip link set dev eth2 xdp obj xdp_ddos_protection.o sec xdp
ip link set dev eth3 xdp obj xdp_ddos_protection.o sec xdp
ip link set dev eth4 xdp obj xdp_ddos_protection.o sec xdp
```

Verifique se anexou:

```bash
ip link show eth1 | grep xdp
```

Saída esperada: `prog/xdp id XXX tag ... jited`

Saia da vítima:

```bash
exit
```

---

## 🔍 Monitorar os mapas BPF (ver IPs bloqueados)

Primeiro descubra os IDs dos mapas (rode no terminal da VM):

```bash
sudo bpftool prog show | grep xdp
```

Anote o `id` do programa, depois:

```bash
sudo bpftool prog show id <ID_DO_PROGRAMA>
```

Anote os `map_ids`. Depois monitore em tempo real:

```bash
watch -n 1 'sudo bpftool map dump id <MAP_ID_1>; sudo bpftool map dump id <MAP_ID_2>'
```

> Na sessão documentada aqui, os IDs foram `59` e `60`, mas podem mudar a cada deploy.

---

## ⚔️ Disparar ataques

### Com 1 atacante

**Abra o Terminal 2:**

```bash
sudo docker exec -it clab-xdp-ddos-atacante1 hping3 --icmp -i u100 10.0.1.1
```

### Com 2 atacantes

**Terminal 2:**
```bash
sudo docker exec -it clab-xdp-ddos-atacante1 hping3 --icmp -i u100 10.0.1.1
```

**Terminal 3:**
```bash
sudo docker exec -it clab-xdp-ddos-atacante2 hping3 --icmp -i u100 10.0.2.1
```

### Com 3 atacantes

Abra um **Terminal 4** adicional:

```bash
sudo docker exec -it clab-xdp-ddos-atacante3 hping3 --icmp -i u100 10.0.3.1
```

### Com 4 atacantes

Abra um **Terminal 5** adicional:

```bash
sudo docker exec -it clab-xdp-ddos-atacante4 hping3 --icmp -i u100 10.0.4.1
```

---

## 📊 O que esperar ver

No terminal do monitor (`watch bpftool map dump`):

```json
[{
    "key": 33619978,
    "value": 1         ← IP na blacklist (bloqueado)
}]
[{
    "key": 33619978,
    "value": {
        "last_update": 0,
        "packet_count": 4    ← contagem de pacotes
    }
}]
```

No terminal do atacante:

```
117076 packets transmitted, 3 packets received, 100% packet loss
```

> **100% packet loss** = XDP bloqueando com sucesso.

---

## 🛑 Derrubar o lab

```bash
sudo containerlab destroy -t ~/clab-xdp/lab.yml
```

---

## 🔄 Resumo dos terminais

| Terminal | O que rodar |
|----------|-------------|
| Terminal 1 | `watch -n 1 'sudo bpftool map dump id X; sudo bpftool map dump id Y'` |
| Terminal 2 | atacante1: `sudo docker exec -it clab-xdp-ddos-atacante1 hping3 --icmp -i u100 10.0.1.1` |
| Terminal 3 | atacante2: `sudo docker exec -it clab-xdp-ddos-atacante2 hping3 --icmp -i u100 10.0.2.1` |
| Terminal 4 | atacante3: `sudo docker exec -it clab-xdp-ddos-atacante3 hping3 --icmp -i u100 10.0.3.1` |
| Terminal 5 | atacante4: `sudo docker exec -it clab-xdp-ddos-atacante4 hping3 --icmp -i u100 10.0.4.1` |

---

## ⚠️ Problemas comuns

**`XDP program already attached`**
> O XDP já está anexado de uma sessão anterior. Não precisa fazer nada, já está funcionando.

**`Peer MTU is too large`**
> Reduza o MTU antes de anexar o XDP:
> ```bash
> ip link set dev ethX mtu 1500
> ```

**Mapas BPF vazios após ataque**
> Verifique se o hping3 está usando a interface `eth1` (e não `eth0`). O IP alvo deve ser `10.0.X.1`, não `172.20.20.X`.

**`ip: not found` nos atacantes**
> Instale o iproute2:
> ```bash
> sudo docker exec clab-xdp-ddos-atacanteX sh -c "apt-get update -q && apt-get install -yq iproute2 hping3"
> ```
