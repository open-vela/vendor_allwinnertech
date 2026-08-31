# vendor/allwinnertech/chips/r528/drv/CMakeLists.txt
# NuttX-style driver wrappers for Allwinner R528 HAL

set(DRV_SRCS)

if(CONFIG_DRIVERS_CE)
    list(APPEND DRV_SRCS ce/drv_ce.c ce/hw_hash.c ce/hw_aes.c)
endif()

if(CONFIG_DRIVERS_GPADC)
    list(APPEND DRV_SRCS gpadc/drv_gpadc.c)
endif()

if(CONFIG_DRIVERS_GPIO)
    list(APPEND DRV_SRCS gpio/drv_gpio.c)
endif()

# ir is unconditionally compiled
list(APPEND DRV_SRCS ir/drv_ir.c)

if(CONFIG_DRIVERS_LRADC)
    list(APPEND DRV_SRCS lradc/drv_lradc.c)
endif()

if(CONFIG_DRIVERS_PWM)
    list(APPEND DRV_SRCS pwm/drv_pwm.c)
endif()

if(CONFIG_DRIVERS_RTC)
    list(APPEND DRV_SRCS rtc/drv_rtc.c)
endif()

if(CONFIG_DRIVERS_SPI)
    list(APPEND DRV_SRCS spi/drv_spi.c)
endif()

if(CONFIG_DRIVERS_TIMER)
    list(APPEND DRV_SRCS timer/drv_timer_ng.c)
endif()

if(CONFIG_DRIVERS_TPADC)
    list(APPEND DRV_SRCS tpadc/drv_tpadc.c)
endif()

if(CONFIG_DRIVERS_TWI)
    list(APPEND DRV_SRCS twi/drv_twi.c)
endif()

if(DRV_SRCS)
    target_sources(arch PRIVATE ${DRV_SRCS})
endif()
