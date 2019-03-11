#include <rte_eal.h>
#include <rte_errno.h>
#include <rte_debug.h>
#include <rte_ethdev.h>
#include <rte_version.h>
#include <rte_ring.h>
#include <stdint.h>
#include <sys/time.h>
#include <inttypes.h>

#define PORT_NBR         0
#define NUMA_SOCK_NBR    0
#define CAPTURE_TIME     10 //in seconds

#define RX_INPUT_PKT_CNT 1024 * 128
#define MBUF_SIZE        RTE_MBUF_DEFAULT_BUF_SIZE
#define CACHE_LINE_SIZE  256
#define RX_MAX_DEQ       64

int test( int argc, char** argv)
{
	rte_log_set_global_level( RTE_LOG_DEBUG );
    if( rte_eal_init( argc, argv ) < 0 ) {
		rte_panic( "EAL initialization failed, error: %s!\n",
		           rte_strerror(rte_errno) );
    }

	printf("Running %s\n", rte_version());

	if( rte_eth_dev_count_avail() == 0 ) {
		rte_panic("No DPDK compatible devices found on this machine!\n");
	}

    if( !rte_eth_dev_is_valid_port( PORT_NBR ) ) {
		rte_panic("The port %d is not valid!\n", PORT_NBR);
	}

	struct rte_eth_conf     port_conf;
	struct rte_eth_dev_info dev_info;
    memset(&port_conf, 0, sizeof( struct rte_eth_conf ));
    memset(&dev_info, 0, sizeof( struct rte_eth_dev_info ));

	rte_eth_dev_info_get(PORT_NBR, &dev_info);

	if( !(dev_info.rx_offload_capa & DEV_RX_OFFLOAD_SCATTER) ) {
		rte_panic("Scatter mode not supported by the port!\n");
	}
	if( !(dev_info.rx_offload_capa & DEV_RX_OFFLOAD_JUMBO_FRAME) ) {
		rte_panic("JF mode not supported by the port!\n");
	}

    port_conf.rxmode.offloads = DEV_RX_OFFLOAD_TIMESTAMP| DEV_RX_OFFLOAD_SCATTER | DEV_RX_OFFLOAD_JUMBO_FRAME;
	port_conf.rxmode.max_rx_pkt_len = 9600;
	port_conf.rxmode.mq_mode = ETH_MQ_RX_NONE;
	port_conf.rx_adv_conf.rss_conf.rss_key = NULL;

	if( rte_eth_dev_configure( PORT_NBR, 1, 0, &port_conf ) < 0 ) {
		rte_panic("Failed to configure the port! Error: %s\n",
		          rte_strerror(rte_errno));
	}

	struct rte_mempool* buffer = rte_pktmbuf_pool_create(
	            "rx_test_pool",
	            RX_INPUT_PKT_CNT,
	            CACHE_LINE_SIZE,
	            0,
	            MBUF_SIZE,
	            NUMA_SOCK_NBR );
	if( !buffer ){
		rte_panic("Failed to allocate pkt mbuf pool, error: %s\n",
		          rte_strerror(rte_errno));
	}

	//Setup the queue
	struct rte_eth_rxconf rx_conf = dev_info.default_rxconf;
	rx_conf.offloads |= port_conf.rxmode.offloads;

	if( rte_eth_rx_queue_setup( PORT_NBR,
	                             0,
	                             RX_INPUT_PKT_CNT,
	                             NUMA_SOCK_NBR,
	                             &rx_conf,
	                             buffer ) < 0){
		rte_panic("Failed to setup the RX queue, error: %s\n",
		          rte_strerror(rte_errno));
	}

	rte_eth_promiscuous_enable( PORT_NBR );

	if( rte_eth_dev_start(PORT_NBR) < 0 ) {
		rte_panic("Failed to start the device, Error: %s\n",
		          rte_strerror(rte_errno));
	}

	struct timeval        tv_epoch, tv_cur;
	struct rte_mbuf*      packets[RX_MAX_DEQ];
	struct rte_mbuf*      pkt = NULL;
	uint16_t              pkt_count = 0;
	uint16_t              idx = 0;
	uint64_t              rx_pkt = 0;
	uint64_t              rx_data = 0;
	struct rte_eth_stats  stats;

	gettimeofday(&tv_epoch,NULL);

	do{
		gettimeofday( &tv_cur, NULL );
		if( tv_cur.tv_sec - tv_epoch.tv_sec >= CAPTURE_TIME )	{
			break;
		}
		pkt_count = rte_eth_rx_burst( PORT_NBR,
		                               0,
		                               packets,
		                               RX_MAX_DEQ );
		if( pkt_count > 0 ) {
			for( idx = 0; idx < pkt_count ; ++idx ){
				pkt = packets[idx];
				printf("Captured pkt, len %d, nb segs %d\n",
				       pkt->pkt_len,
				       pkt->nb_segs);
				rx_data += pkt->pkt_len;
				rte_pktmbuf_free(packets[idx]);
			}
			rx_pkt += pkt_count;
		}
	}while(1);

	printf("Terminating after %"PRIu64" seconds, captured %"PRIu64" packets: %"PRIu64" bytes of data..\n",
	       (tv_cur.tv_sec - tv_epoch.tv_sec),
	       rx_pkt,
	       rx_data);
	if( !rte_eth_stats_get(PORT_NBR, &stats) ) {
		printf( "Port %d stats: ipkt:%"PRIu64",opkt:%"PRIu64",ierr:%"PRIu64",oerr:%"PRIu64",imiss:%"PRIu64"\n",
		     PORT_NBR,
		     stats.ipackets, stats.opackets,
		     stats.ierrors, stats.oerrors,
		     stats.imissed );
	}

	rte_eth_dev_stop( PORT_NBR );

	if( buffer) {
		rte_mempool_free( buffer );
	}
	return 0;
}
