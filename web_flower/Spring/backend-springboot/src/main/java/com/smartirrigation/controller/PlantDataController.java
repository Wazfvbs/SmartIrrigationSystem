package com.smartirrigation.controller;

import com.smartirrigation.dto.PlantDataDTO;
import com.smartirrigation.dto.Result;
import com.smartirrigation.entity.PlantData;
import com.smartirrigation.repository.PlantDataRepository;
import com.smartirrigation.service.PlantDataService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.domain.PageRequest;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.time.LocalDateTime;
import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api/plant")
@CrossOrigin
public class PlantDataController {

    @Autowired
    private PlantDataService service;

    @PostMapping("/upload")
    public ResponseEntity<?> upload(@RequestBody PlantDataDTO dto) {
        if (dto.getPlant_id() == null || dto.getTimestamp() == null) {
            return ResponseEntity.badRequest().body(
                    new Response(400, "Missing plant_id or timestamp")
            );
        }
        try {
            service.savePlantData(dto.getPlant_id(), dto);
            return ResponseEntity.ok(new Response(200, "Upload successful"));
        } catch (Exception e) {
            return ResponseEntity.status(500).body(new Response(500, "Error: " + e.getMessage()));
        }
    }
    @GetMapping("/history")
    public ResponseEntity<?> getHistory(@RequestParam String plant_id) {
        if (plant_id == null || plant_id.isEmpty()) {
            return ResponseEntity.badRequest().body(Map.of(
                    "code", 400,
                    "message", "缺少 plant_id",
                    "data", null
            ));
        }

        try {
            List<PlantData> dataList = service.getHistory(plant_id);
            return ResponseEntity.ok(Map.of(
                    "code", 200,
                    "message", "OK",
                    "data", dataList
            ));
        } catch (Exception e) {
            return ResponseEntity.status(500).body(Map.of(
                    "code", 500,
                    "message", "查询失败: " + e.getMessage(),
                    "data", null
            ));
        }
    }



    @Autowired
    private PlantDataRepository plantDataRepository;

    @GetMapping("/latest")
    public ResponseEntity<?> getLatestData(@RequestParam("plant_id") String plantId) {
        if (plantId == null || plantId.isEmpty()) {
            return ResponseEntity.badRequest().body(Map.of(
                    "code", 400,
                    "message", "缺少 plant_id 参数"
            ));
        }

        return plantDataRepository.findFirstByPlantIdOrderByTimestampDesc(plantId)
                .map(data -> ResponseEntity.ok(Map.of(
                        "code", 200,
                        "message", "OK",
                        "data", data
                )))
                .orElse(ResponseEntity.status(404).body(Map.of(
                        "code", 404,
                        "message", "未找到该植物的数据"
                )));
    }
    record Response(int code, String message) {}
}
