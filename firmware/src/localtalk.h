#pragma once

void localtalkSend(uint8_t *data, int len);
void localtalkInit();
void localtalkTick();
void localtalk_send_llap_resp(uint8_t node);
