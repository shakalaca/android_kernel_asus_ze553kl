ccflags-y += -Idrivers/media/platform/msm/camera_v2
ccflags-y += -Idrivers/media/platform/msm/camera_v2/common
ccflags-y += -Idrivers/media/platform/msm/camera_v2/msm_vb2
ccflags-y += -Idrivers/media/platform/msm/camera_v2/camera
ccflags-y += -Idrivers/media/platform/msm/camera_v2/sensor/io
ccflags-y += -Idrivers/media/platform/msm/camera_v2/sensor/cci
ifeq ($(PRJ_ID),$(ASUS_ZE553KL)) 
obj-$(CONFIG_MSMB_CAMERA) += cci/ io/ csiphy/ csid/ actuator/ eeprom/  flash/ laser_focus/ ois/ ois-rumba/
else
obj-$(CONFIG_MSMB_CAMERA) += cci/ io/ csiphy/ csid/ actuator/ eeprom/  flash/
endif
obj-$(CONFIG_MSM_CAMERA_SENSOR) += msm_sensor_init.o msm_sensor_driver.o msm_sensor.o fac_camera.o
