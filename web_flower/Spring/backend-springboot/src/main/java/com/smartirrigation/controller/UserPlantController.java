package com.smartirrigation.controller;

import com.smartirrigation.dto.PlantInfoDTO;
import com.smartirrigation.entity.PlantProfile;
import com.smartirrigation.entity.PlantSpecies;
import com.smartirrigation.entity.UserPlant;
import com.smartirrigation.repository.PlantProfileRepository;
import com.smartirrigation.repository.PlantSpeciesRepository;
import com.smartirrigation.repository.UserPlantRepository;
import com.smartirrigation.service.UserPlantService;
import com.smartirrigation.util.JwtUtil;
import jakarta.servlet.http.HttpServletRequest;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.ResponseEntity;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.*;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

@RestController
@RequestMapping("/api/user")
public class UserPlantController {

    @Autowired
    private UserPlantService userPlantService;
    @Autowired
    private JwtUtil jwtUtil;
    @Autowired
    private UserPlantRepository userPlantRepository;
    @Autowired
    private PlantProfileRepository plantProfileRepository;

    @Autowired
    private PlantSpeciesRepository plantSpeciesRepository;

    @GetMapping("/plants")
    @PreAuthorize("hasRole('USER')")
    public ResponseEntity<?> getMyPlants(@RequestHeader("Authorization") String token) {
        Long uid = jwtUtil.extractUserId(token);
        System.out.println("✅ 当前用户 ID：" + uid);

        List<UserPlant> bindings = userPlantRepository.findByUserId(uid);
        System.out.println("✅ 查询到绑定记录数：" + bindings.size());

        List<Map<String, Object>> result = bindings.stream().map(binding -> {
            String plantId = binding.getPlantId();
            PlantProfile profile = plantProfileRepository.findById(plantId).orElse(null);

            String nickname = profile != null ? profile.getNickname() : "植物" + plantId;
            String species = profile != null ? profile.getSpecies() : "未知";

            PlantSpecies speciesInfo = plantSpeciesRepository.findById(species).orElse(null);
            String image_url = speciesInfo != null ? speciesInfo.getImage_url() : "default.png";

            Map<String, Object> map = new HashMap<>();
            map.put("plant_id", plantId);
            map.put("nickname", nickname);
            map.put("species", species);
            map.put("image_url", image_url);
            return map;
        }).collect(Collectors.toList());

        return ResponseEntity.ok(result);
    }


    @PostMapping("/plant/bind")
    public ResponseEntity<?> bind(@RequestBody Map<String, String> body, @RequestHeader("Authorization") String authHeader) {

        Long userId = jwtUtil.extractUserIdFromToken(authHeader);
        String plantId = body.get("plant_id");
        userPlantService.bindPlant(userId, plantId);
        return ResponseEntity.ok(Map.of("msg", "绑定成功"));
    }
}
