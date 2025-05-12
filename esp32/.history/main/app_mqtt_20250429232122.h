#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H
#pragma once
void mqtt_client_start(void);
void mqtt_publish_report(const char *payload);
#endif // MQTT_CLIENT_H
