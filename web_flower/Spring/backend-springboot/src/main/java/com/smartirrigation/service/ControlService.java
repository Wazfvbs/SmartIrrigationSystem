package com.smartirrigation.service;

import com.smartirrigation.dto.ControlCommandDTO;
import com.smartirrigation.entity.ControlLog;
import com.smartirrigation.repository.ControlLogRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.time.LocalDateTime;

@Service
public class ControlService {

    @Autowired
    private ControlLogRepository logRepository;

    public void processCommand(ControlCommandDTO dto) {
        ControlLog log = new ControlLog();
        log.setPlantId(dto.getPlant_id());
        log.setCmd(dto.getCmd());
        log.setStatus("已发送");  // 可设为：待发送 / 成功 / 失败
        log.setTimestamp(LocalDateTime.now());
        logRepository.save(log);

        // TODO: 转发指令到 ESP32，如 MQTT、HTTP、WebSocket 等
    }
}
