package com.smartirrigation.repository;

import com.smartirrigation.entity.ControlLog;
import org.springframework.data.jpa.repository.JpaRepository;

public interface ControlLogRepository extends JpaRepository<ControlLog, Long> {
}
