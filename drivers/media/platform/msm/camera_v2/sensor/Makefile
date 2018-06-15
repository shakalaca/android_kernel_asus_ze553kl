ccflags-y += -Idrivers/media/platform/msm/camera_v2
ccflags-y += -Idrivers/media/platform/msm/camera_v2/common
ccflags-y += -Idrivers/media/platform/msm/camera_v2/msm_vb2
ccflags-y += -Idrivers/media/platform/msm/camera_v2/camera
ccflags-y += -Idrivers/media/platform/msm/camera_v2/sensor/io
ccflags-y += -Idrivers/media/platform/msm/camera_v2/sensor/cci
obj-$(CONFIG_MSMB_CAMERA) += cci/ io/ csiphy/ csid/ actuator/ eeprom/ flash/ #ois/ ir_led/ ir_cut/
ifeq ($(TARGET_PRODUCT), Z01H)
obj-$(CONFIG_MSMB_CAMERA) += ois-rumba/ laser_focus/
endif
ifeq ($(TARGET_PRODUCT), Z018)
obj-$(CONFIG_MSMB_CAMERA) += ois/ laser_focus/
endif
obj-$(CONFIG_MSM_CAMERA_SENSOR) += msm_sensor_init.o msm_sensor_driver.o msm_sensor.o fac_camera.o
