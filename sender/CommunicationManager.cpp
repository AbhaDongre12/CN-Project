#include "CommunicationManager.h"

CommunicationManager::CommunicationManager(){
    uint8_t address[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    for(int i=0;i<6;i++){
        broadcastAddress[i]=address[i];
    }
}

bool CommunicationManager::begin(){
    WiFi.mode(WIFI_STA);

    if(esp_now_init()!=ESP_OK){
        return false;
    }

    esp_now_peer_info_t peerInfo={};
    
    for(int i=0;i<6;i++){
        peerInfo.peer_addr[i]=broadcastAddress[i];
    }

    peerInfo.channel=0;
    peerInfo.encrypt=false;

    if(esp_now_is_peer_exist(broadcastAddress)==false){
        if(esp_now_add_peer(&peerInfo)!=ESP_OK){
            return false;
        }
    }

    return true;
}

bool CommunicationManager::sendMessage(const BlindLinkMessage& message){
    if(message.type==MESSAGE_SUPPRESS){
        return false;
    }

    esp_err_t result=esp_now_send(
        broadcastAddress,
        (uint8_t*)&message,
        sizeof(BlindLinkMessage)
    );

    return result==ESP_OK;
}

bool CommunicationManager::sendClear(uint8_t object_id){
    BlindLinkMessage message;

    message.type=MESSAGE_CLEAR;
    message.vehicle_id=0;
    message.object_id=object_id;
    message.changed_fields=0;
    message.distance_cm=0;
    message.zone=ZONE_LEFT;
    message.sequence_number=0;

    return sendMessage(message);
}

bool CommunicationManager::sendHeartbeat(){
    BlindLinkMessage message;

    message.type=MESSAGE_HEARTBEAT;
    message.vehicle_id=0;
    message.object_id=0;
    message.changed_fields=0;
    message.distance_cm=0;
    message.zone=ZONE_LEFT;
    message.sequence_number=0;

    return sendMessage(message);
}

void CommunicationManager::displayMacAddress(){
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
}