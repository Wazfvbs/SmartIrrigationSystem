package com.smartirrigation.service;

import com.smartirrigation.dto.PlantDataDTO;
import com.smartirrigation.entity.PlantData;
import com.smartirrigation.repository.PlantDataRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import java.time.LocalDateTime;
import java.time.format.DateTimeParseException;
import java.util.List;

@Service
public class PlantDataService {

    @Autowired
    private PlantDataRepository repository;

    public void savePlantData(String plantId, PlantDataDTO dto) {
        PlantData data = new PlantData();
        data.setPlantId(plantId);
        try {
            data.setTimestamp(LocalDateTime.parse(dto.getTimestamp()));
        } catch (DateTimeParseException e) {
            throw new IllegalArgumentException("Invalid timestamp format");
        }
        data.setTemperature(dto.getTemperature());
        data.setHumidity(dto.getHumidity());
        data.setSoil_moisture(dto.getSoil_moisture());
        data.setLight(dto.getLight());
        data.setWater_level(dto.getWater_level());
        data.setBattery(dto.getBattery());
        data.setMode(dto.getMode());
        repository.save(data);
    }
    public List<PlantData> getHistory(String plantId) {
        return repository.findByPlantIdOrderByTimestampDesc(plantId);
    }
}
