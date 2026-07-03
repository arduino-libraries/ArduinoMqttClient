
#if defined(__ZEPHYR__) && __ZEPHYR__ == 1 && defined(CONFIG_MQTT_LIB)
#include "ZephyrMqttClient.h"
#include <zephyr/random/random.h>
#include <Arduino.h>

extern "C" {
	/** Handler for asynchronous MQTT events */
	static void _mqtt_event_handler(struct mqtt_client *const client, const struct mqtt_evt *evt) {
		((ZephyrMqttClient*)client->user_data)->mqtt_event_handler(
			client,
			evt);
	}
}

static inline uint8_t convertQosValue(MqttQos qos) {
	return static_cast<uint8_t>(qos);
}

/* This helper function is used to generalize the allocation and deallocation
 * for zephyr mqtt_utf8 structs. It deallocates the pointed to object if no value is provided
 * it allocates the struct if a value is provided
 * mqtt_topic could also be used, since the struct respects mqtt_utf8 fields
 */
static inline void __mqtt_utf8_allocation_helper(
		struct mqtt_utf8** s,
		const uint8_t* value=nullptr,
		size_t size=0,
		size_t struct_size=sizeof(struct mqtt_utf8)) {

	if(value == nullptr) {
		free(*s);
		*s = nullptr;
	} else {
		if(*s == nullptr) {
			*s = (struct mqtt_utf8*) malloc(struct_size);
		}

		(*s)->utf8 = (const uint8_t*)value;
		(*s)->size = size;
	}
}

ZephyrMqttClient::ZephyrMqttClient()
: last_message(nullptr) {
	fds.fd = -1;
    mqtt_client_init(&client);
}

ZephyrMqttClient::~ZephyrMqttClient() {
	__mqtt_utf8_allocation_helper(&client.user_name);
	__mqtt_utf8_allocation_helper(&client.password);
	__mqtt_utf8_allocation_helper((struct mqtt_utf8**)&client.will_topic);
	__mqtt_utf8_allocation_helper(&client.will_message);
}

int ZephyrMqttClient::connect(IPAddress ip, uint16_t port) {
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;
	broker4->sin_addr.s_addr = (uint32_t)ip;

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(port);

	return connect(broker);
}

int ZephyrMqttClient::connect(const char *host, uint16_t port) {
	int rc = 0;
	struct addrinfo *result;
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;

	const struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};

    /* Resolve IP address of MQTT broker */
	// FIXME port should be a string
	rc = getaddrinfo(host, "1883", &hints, &result);
	if (rc != 0) {
		return -EIO;
	}

	// TODO this may not be compatible with ipv6?
	broker4->sin_addr.s_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(port);

    freeaddrinfo(result);

	return connect(broker);
}

int ZephyrMqttClient::connect(sockaddr_storage addr) {
	client_init();

	int rc = 0;

	memcpy(&broker, &addr, sizeof(broker));
	client.broker = &broker;

	rc = mqtt_connect(&client);
	if (rc != 0) {
		// TODO LOG ERROR?
		return rc;
	}

	if (client.transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds.fd = client.transport.tcp.sock;
	}else if (client.transport.type == MQTT_TRANSPORT_SECURE) {
		fds.fd = client.transport.tls.sock;
	}

	fds.events = ZSOCK_POLLIN;

	// FIXME this requires the timeout to be properly configured in the poll function
	poll(); // make the client connect
	return rc;
}

void ZephyrMqttClient::client_init() {
	client.protocol_version = MQTT_VERSION_3_1_1; // FIXME provide a way to set these values

	// client.client_id.utf8 = (const uint8_t*)_clientid; // TODO generate client_id if not provided
	// client.client_id.size = strlen((const char*) _clientid);

	/* MQTT buffers configuration */
	client.rx_buf = rx_buffer;
	client.rx_buf_size = sizeof(rx_buffer);
	client.tx_buf = tx_buffer;
	client.tx_buf_size = sizeof(tx_buffer);

    client.user_data = this;
	client.evt_cb = _mqtt_event_handler;

	// TODO TLS settings
	client.transport.type = MQTT_TRANSPORT_NON_SECURE;

	// client.transport.type = MQTT_TRANSPORT_SECURE;
	// struct mqtt_sec_config *tls_config = &client.transport.tls.config;

	// tls_config->peer_verify = TLS_PEER_VERIFY_REQUIRED;
	// tls_config->cipher_list = NULL;
	// // tls_config->cipher_list = mbedtls_cipher_list();
	// // tls_config->cipher_count = mbedtls_cipher_list();
	// tls_config->sec_tag_list = m_sec_tags;
	// tls_config->sec_tag_count = 2;
	// tls_config->hostname = "iot.arduino.cc";
	// // tls_config->cert_nocopy = TLS_CERT_NOCOPY_NONE;
}

void ZephyrMqttClient::disconnect() { // TODO call this in the destructor
	mqtt_disconnect(&client, nullptr); // TODO mqtt_disconnect_param takes values in case of mqtt version 5

	close(fds.fd); // FIXME is this correct?
	fds.fd = -1;
}

uint8_t ZephyrMqttClient::connected() {
	return fds.fd != -1; // FIXME is this enough? nope
}

ZephyrMqttClient::operator bool() {
	return connected() == 1;
}

error_t ZephyrMqttClient::subscribe(Topic t, MqttQos qos) {
	// TODO allow user to subscribe to multiple topics

	struct mqtt_topic topics[] = {{
		.topic = {
			.utf8 = (const uint8_t*) t,
			.size = strlen(t)
		},
		.qos = convertQosValue(qos),
	}};

	const struct mqtt_subscription_list sub_list {
		.list = topics,
		.list_count = 1,
		.message_id = sys_rand16_get()
	};

	return mqtt_subscribe(&client, &sub_list); // should this parameter kept in an available storage
}

error_t ZephyrMqttClient::publish(Topic t, uint8_t payload[], size_t size, MqttQos qos, MqttPublishFlag flags) {
	struct mqtt_publish_param param;

	param.message.topic.qos = convertQosValue(qos);
	param.message.topic.topic.utf8 = (uint8_t *)t;
	param.message.topic.topic.size = strlen(t);
	param.message.payload.data = payload;
	param.message.payload.len = size;
	param.message_id = sys_rand16_get(); // TODO what value should this take?
	param.dup_flag = (flags & DupEnabled) == DupEnabled;
	param.retain_flag = (flags & RetainEnabled) == RetainEnabled;

	return mqtt_publish(&client, &param); // error codes should be proxied
}

error_t ZephyrMqttClient::unsubscribe(Topic t) {
	struct mqtt_topic topic;
	struct mqtt_subscription_list unsub;

	topic.topic.utf8 = (const uint8_t*)t;
	topic.topic.size = strlen(t);
	unsub.list = &topic;
	unsub.list_count = 1U;
	unsub.message_id = sys_rand16_get();

	return mqtt_unsubscribe(&client, &unsub);
}

void ZephyrMqttClient::poll() {
	int rc;

	// TODO This defines the minimum time poll should wait till another event occurs, find a proper value
	// auto _mqtt_keepalive_time_left = mqtt_keepalive_time_left(&client);
	auto _mqtt_keepalive_time_left = 100; // poll returns immediately if no event occurs

	rc = zsock_poll(&fds, 1, _mqtt_keepalive_time_left);
	if (rc < 0) {
		// LOG_ERR("Socket poll error [%d]", rc);
		return;
	}

	// FIXME understand what this does

	if (rc >= 0) {
		if (fds.revents & ZSOCK_POLLIN) {
			/* MQTT data received */
			rc = mqtt_input(&client);
			if (rc != 0) {
				// LOG_ERR("MQTT Input failed [%d]", rc);
				// return rc;
				return;
			}

			/* Socket error */
			if (fds.revents & (ZSOCK_POLLHUP | ZSOCK_POLLERR)) {
				// LOG_ERR("MQTT socket closed / error");
				// return -ENOTCONN;
			}
		}
	} else {
		/* Socket poll timed out, time to call mqtt_live() */
		rc = mqtt_live(&client);
		if (rc != 0) {
			// LOG_ERR("MQTT Live failed [%d]", rc);
			// return rc;
			return;
		}
	}
	// return rc;
}

error_t ZephyrMqttClient::ping() {
	return mqtt_ping(&client);
}

int ZephyrMqttClient::read() {
	uint8_t a;
	int res = read(&a, 1);
	return res <= 0 ? res : a;
}

int ZephyrMqttClient::read(uint8_t payload[], size_t len) {
	int res = mqtt_read_publish_payload_blocking(&client, payload, len);

	if(res < 0) {
		return res;
	}

	if(available() == 0) {
		if (messageQoS() == MqttQos1) {
			const struct mqtt_puback_param ack_param = {
				.message_id = messageId()
			};
			mqtt_publish_qos1_ack(&client, &ack_param);
		} else if (messageQoS() == MqttQos2) {
			const struct mqtt_pubrec_param rec_param = {
				.message_id = messageId()
			};

			mqtt_publish_qos2_receive(&client, &rec_param);
		}

		delete last_message;
		last_message = nullptr;
	}

	return res;
}

int ZephyrMqttClient::available() {
	return client.internal.remaining_payload;
}

void ZephyrMqttClient::mqtt_event_handler(
    struct mqtt_client *const client, const struct mqtt_evt *evt) {

	switch (evt->type) {
	case MQTT_EVT_PUBREC: {
		if (evt->result != 0) {
			break;
		}

		const struct mqtt_pubrel_param rel_param = {
			.message_id = evt->param.pubrec.message_id
		};

		mqtt_publish_qos2_release(client, &rel_param);
		break;
	}
	case MQTT_EVT_PUBREL: {
		if (evt->result != 0) {
			break;
		}

		const struct mqtt_pubcomp_param rec_param = {
			.message_id = evt->param.pubrel.message_id
		};

		mqtt_publish_qos2_complete(client, &rec_param);
		break;
	}
	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *p = &evt->param.publish;
		if(last_message == nullptr) {
			last_message = new mqtt_publish_param;
		}

		memcpy(last_message, p, sizeof(*last_message));

		// Pass the stream to the user callback
		if(_cbk != nullptr) {
			const char* topic = (const char*)p->message.topic.topic.utf8; // will this be null terminated string?

			_cbk(topic);
		}
	}
	default:
		break;
	}
}

void ZephyrMqttClient::setId(const char* client_id) {
	client.client_id.utf8 = (const uint8_t*)client_id;
	client.client_id.size = strlen((const char*)client_id);
}

void ZephyrMqttClient::setUsernamePassword(const char* username, const char* password) {
	__mqtt_utf8_allocation_helper(&client.user_name, (const uint8_t*)username, strlen(username));
	__mqtt_utf8_allocation_helper(&client.password, (const uint8_t*)password, strlen(password));
}

void ZephyrMqttClient::setWill(
		Topic will_topic, const uint8_t* will_message, size_t will_size, MqttQos qos,
		MqttPublishFlag flags) { // TODO use publish flags
	__mqtt_utf8_allocation_helper(
		(struct mqtt_utf8**)&client.will_topic, (const uint8_t*)will_topic,
		strlen(will_topic), sizeof(struct mqtt_topic));
	client.will_topic->qos = qos;

	__mqtt_utf8_allocation_helper(&client.will_message, will_message, will_size);

}

void ZephyrMqttClient::setReceiveCallback(MqttReceiveCallback cbk) {
    this->_cbk = cbk;
}

String ZephyrMqttClient::messageTopic() const {
	if(last_message != nullptr) {
		return (const char*)last_message->message.topic.topic.utf8;
	}

	return "";
}

int ZephyrMqttClient::messageDup() const {
	if(last_message != nullptr) {
		return last_message->dup_flag;
	}

	return -1;
}

uint16_t ZephyrMqttClient::messageId() const {
	if(last_message != nullptr) {
		return last_message->message_id;
	}

	return 0;
}

MqttQos ZephyrMqttClient::messageQoS() const {
	if(last_message != nullptr) {
		return static_cast<MqttQos>(last_message->message.topic.qos);
	}

	return QosDefault;
}

int ZephyrMqttClient::messageRetain() const {
	if(last_message != nullptr) {
		return last_message->retain_flag;
	}

	return -1;
}


#endif // CONFIG_MQTT_LIB
