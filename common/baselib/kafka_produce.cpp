#include <ctype.h>  
#include <signal.h>  
#include <string.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include <stdio.h>  
#include <syslog.h>  
#include <sys/time.h>  
#include <errno.h>  

#include "kafka_produce.h"  

using namespace std;



static void logger(const rd_kafka_t *rk, int level, const char *fac, const char *buf)
{
}


int ProducerKafka::init_kafka(int partition, const char *brokers, const char *topic)
{
	char tmp[16] = { 0 };
	char errstr[512] = { 0 };

	partition_ = partition;

	/* Kafka configuration */
	conf_ = rd_kafka_conf_new();

	//set logger :register log function  
	rd_kafka_conf_set_log_cb(conf_, logger);

	/* Quick termination */
	// snprintf(tmp, sizeof(tmp), "%i", SIGIO);
	// rd_kafka_conf_set(conf_, "internal.termination.signal", tmp, NULL, 0);

	/*topic configuration*/
	topic_conf_ = rd_kafka_topic_conf_new();

	if (!(handler_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf_, errstr, sizeof(errstr))))
	{
		fprintf(stderr, "*****Failed to create new producer: %s*******\n", errstr);
		return -1;
	}

	// rd_kafka_set_log_level(handler_, LOG_DEBUG);

	/* Add brokers */
	if (rd_kafka_brokers_add(handler_, brokers) == 0)
	{
		fprintf(stderr, "****** No valid brokers specified********\n");
		return -1;
	}


	/* Create topic */
	topic_ = rd_kafka_topic_new(handler_, topic, topic_conf_);

	return 0;
}

void ProducerKafka::destroy()
{
	/* Destroy topic */
	rd_kafka_topic_destroy(topic_);

	/* Destroy the handle */
	rd_kafka_destroy(handler_);
}

int ProducerKafka::push_data_to_kafka(const char* buffer, const int buf_len)
{
	int ret;

	if (!handler_ || !topic_)
	{
		return -1;
	}

	if (NULL == buffer)
		return 0;

	ret = rd_kafka_produce(topic_, RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY,
		(void*)buffer, (size_t)buf_len, NULL, 0, NULL);

	if (ret == -1)
	{
//		fprintf(stderr, "****Failed to produce to topic %s partition %i: %s*****\n",
//			rd_kafka_topic_name(topic_), partition_,
//			rd_kafka_err2str(rd_kafka_errno2err(errno)));

		// rd_kafka_poll(handler_, 0);
		return -1;
	}

	// rd_kafka_poll(handler_, 0);
	return 0;
}



