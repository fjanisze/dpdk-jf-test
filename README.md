# dpdk-jf-test
Test code for the JF issue with DPDK

Before attempting to build the test code, make sure RTE_SDK is defined.

Apparently the same identical piece of software works and capture jumbo frame while compile
with $(RTE_SDK)/mk/rte.extapp.mk but it does not work while compiled
with $(RTE_SDK)/mk/rte.lib.mk, by that I mean that I cannot capture
Jumbo Frames.

In the ./app folder the code is built as regular application using a DPDK
makefile which calls $(RTE_SDK)/mk/rte.extapp.mk.

in the ./lib folder you have the same piece of code, but instead of
being inside the main(int, char**) function it is placed inside the lib
within the test(int,char**) function, to call the test function from the
library I've build a little test.c piece of code that do the following:

.
extern int test(int argc, char**argv);

int main(int argc, char** argv)
{
    return test(argc,argv);
}
.

I build the test.c application with:

 gcc -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lmlx4 -lmlx5 -lm
-libverbs -lrte_eal -L. -lpthread -lnuma -ldl test.c -ljftest -o test

please note that jftest.a must be present in the building dir, this .a
file is create by calling make inside ./lib (where the DPDK Makefile is
present).

If you make sure to configure RTE_SDK then you can build the two test
application by:

A) Entering ./app and calling make
B) Entering ./lib and calling make, then copying from ./lib/build/lib/
the newly create libjftest.a to the ./lib folder and calling the command
I show you above to build the test executable:

 gcc -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lmlx4 -lmlx5 -lm
-libverbs -lrte_eal -L. -lpthread -lnuma -ldl test.c -ljftest -o test

Please note that the code in ./app/main.c and ./lib/main.c is absolutely
identical, the only different is that one is build as a lib (.a) and the
second as ELF (executable).

The build from A can capture Jumbo Frame, the build from B cannot
capture Jumbo Frames.

I've a test PCAP with two packets, one of size 9022 bytes the other
1522, I send those packets in a loop to my box to test the output of the
two test applications, on my development box (A) is generating the
following output:

.
EAL: Detected 28 lcore(s)
EAL: Detected 1 NUMA nodes
EAL: Multi-process socket /var/run/dpdk/rte/mp_socket
EAL: No free hugepages reported in hugepages-1048576kB
EAL: Probing VFIO support...
EAL: PCI device 0000:00:1f.6 on NUMA socket 0
EAL:   probe driver: 8086:15b8 net_e1000_em
EAL: PCI device 0000:b3:00.0 on NUMA socket 0
EAL:   probe driver: 15b3:1015 net_mlx5
net_mlx5: MPLS over GRE/UDP tunnel offloading disabled due to old
OFED/rdma-core version or firmware configuration
EAL: PCI device 0000:b3:00.1 on NUMA socket 0
EAL:   probe driver: 15b3:1015 net_mlx5
net_mlx5: MPLS over GRE/UDP tunnel offloading disabled due to old
OFED/rdma-core version or firmware configuration
Running DPDK 19.02.0
Captured pkt, len 9022, nb segs 5
Captured pkt, len 1522, nb segs 1
Captured pkt, len 9022, nb segs 5
Captured pkt, len 1522, nb segs 1
Captured pkt, len 9022, nb segs 5
Captured pkt, len 1522, nb segs 1
Captured pkt, len 9022, nb segs 5
Captured pkt, len 1522, nb segs 1
.
. <CUT>

(B) is generating the following output:

.
EAL: Detected 28 lcore(s)
EAL: Detected 1 NUMA nodes
EAL: Multi-process socket /var/run/dpdk/rte/mp_socket
EAL: No free hugepages reported in hugepages-1048576kB
EAL: Probing VFIO support...
EAL: PCI device 0000:00:1f.6 on NUMA socket 0
EAL:   probe driver: 8086:15b8 net_e1000_em
EAL: PCI device 0000:b3:00.0 on NUMA socket 0
EAL:   probe driver: 15b3:1015 net_mlx5
net_mlx5: MPLS over GRE/UDP tunnel offloading disabled due to old
OFED/rdma-core version or firmware configuration
EAL: PCI device 0000:b3:00.1 on NUMA socket 0
EAL:   probe driver: 15b3:1015 net_mlx5
net_mlx5: MPLS over GRE/UDP tunnel offloading disabled due to old
OFED/rdma-core version or firmware configuration
Running DPDK 19.02.0
Captured pkt, len 1522, nb segs 1
Captured pkt, len 1522, nb segs 1
Captured pkt, len 1522, nb segs 1
Captured pkt, len 1522, nb segs 1
Captured pkt, len 1522, nb segs 1
.<CUT>

The same piece of software build in two different way generate two
different outputs.

Any idea why?
