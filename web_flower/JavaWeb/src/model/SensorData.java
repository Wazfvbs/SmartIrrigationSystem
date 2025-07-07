package model;

import java.util.Date;

/**
 * SensorData - 对应 sensor_data 表
 */
public class SensorData {
    private int id;
    private String deviceId;
    private double temperature;
    private double humidity;
    private double soilMoisture;
    private double waterLevel;
    private Date timestamp;

    // Getter & Setter
    public int getId() {
        return id;
    }
    public void setId(int id) {
        this.id = id;
    }
    public String getDeviceId() {
        return deviceId;
    }
    public void setDeviceId(String deviceId) {
        this.deviceId = deviceId;
    }
    public double getTemperature() {
        return temperature;
    }
    public void setTemperature(double temperature) {
        this.temperature = temperature;
    }
    public double getHumidity() {
        return humidity;
    }
    public void setHumidity(double humidity) {
        this.humidity = humidity;
    }
    public double getSoilMoisture() {
        return soilMoisture;
    }
    public void setSoilMoisture(double soilMoisture) {
        this.soilMoisture = soilMoisture;
    }
    public double getWaterLevel() {
        return waterLevel;
    }
    public void setWaterLevel(double waterLevel) {
        this.waterLevel = waterLevel;
    }
    public Date getTimestamp() {
        return timestamp;
    }
    public void setTimestamp(Date timestamp) {
        this.timestamp = timestamp;
    }
}

