#ifndef KAFKA_PRODUCE_H
#define KAFKA_PRODUCE_H

#include <rdkafka.h>

//const int PRODUCER_INIT_FAILED = -1;  
//const int PRODUCER_INIT_SUCCESS = 0;  
//const int PUSH_DATA_FAILED = -1;  
//const int PUSH_DATA_SUCCESS = 0;  

class ProducerKafka
{
public:
	ProducerKafka(){};
	~ProducerKafka(){}

	int init_kafka(int partition, const char *brokers, const char *topic);
	int push_data_to_kafka(const char* buf, const int buf_len);
	void destroy();

private:
	int partition_;

	//rd  
	rd_kafka_t * handler_;
	rd_kafka_conf_t * conf_;

	//topic  
	rd_kafka_topic_t * topic_;
	rd_kafka_topic_conf_t * topic_conf_;
};
#endif 
