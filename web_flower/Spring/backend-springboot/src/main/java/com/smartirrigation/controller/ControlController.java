package com.smartirrigation.controller;

import com.smartirrigation.dto.ControlCommandDTO;
import com.smartirrigation.service.ControlService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.Map;

@RestController
@RequestMapping("/api/control")
@CrossOrigin
public class ControlController {

    @Autowired
    private ControlService controlService;

    @PostMapping("/send")
    public ResponseEntity<?> sendCommand(@RequestBody ControlCommandDTO dto) {
        if (dto.getPlant_id() == null || dto.getCmd() == null) {
            return ResponseEntity.badRequest().body(Map.of(
                    "code", 400,
                    "message", "缺少参数 plant_id 或 cmd"
            ));
        }

        controlService.processCommand(dto);
        return ResponseEntity.ok(Map.of(
                "code", 200,
                "message", "Command sent successfully"
        ));
    }
}
