#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory(socket)
        
if (CONFIG_CONNECT_WITH_WIFI)
        add_subdirectory(wifi)
endif()
