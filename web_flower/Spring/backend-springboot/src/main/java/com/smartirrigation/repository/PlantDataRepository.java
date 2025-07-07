package com.smartirrigation.repository;

import com.smartirrigation.entity.PlantData;
import org.springframework.data.domain.PageRequest;
import org.springframework.data.jpa.repository.JpaRepository;

import java.awt.print.Pageable;
import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;

public interface PlantDataRepository extends JpaRepository<PlantData, Long> {

    // 获取某个 plantId 的所有数据，按时间倒序（用于 /history）
    List<PlantData> findByPlantIdOrderByTimestampDesc(String plantId);

    // 获取某个 plantId 的最新一条数据（用于 /latest）
    Optional<PlantData> findFirstByPlantIdOrderByTimestampDesc(String plantId);

    List<PlantData> findByPlantIdAndTimestampBetweenOrderByTimestampAsc(String plantId, LocalDateTime startTime, LocalDateTime endTime, PageRequest of);
}
