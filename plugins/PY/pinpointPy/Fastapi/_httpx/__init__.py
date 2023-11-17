#!/usr/bin/env python
# -*- coding: UTF-8 -*-

# ------------------------------------------------------------------------------
#  Copyright  2020. NAVER Corp.                                                -
#                                                                              -
#  Licensed under the Apache License, Version 2.0 (the "License");             -
#  you may not use this file except in compliance with the License.            -
#  You may obtain a copy of the License at                                     -
#                                                                              -
#   http://www.apache.org/licenses/LICENSE-2.0                                 -
#                                                                              -
#  Unless required by applicable law or agreed to in writing, software         -
#  distributed under the License is distributed on an "AS IS" BASIS,           -
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.    -
#  See the License for the specific language governing permissions and         -
#  limitations under the License.                                              -
# ------------------------------------------------------------------------------

# create by eelu

from pinpointPy.Interceptor import Interceptor, intercept_once
from pinpointPy import get_logger


@intercept_once
def monkey_patch():
    try:
        from httpx import AsyncClient
        from .httpxPlugins import HttpxRequestPlugins
        Interceptors = [
            Interceptor(AsyncClient, 'request', HttpxRequestPlugins)
        ]

        for interceptor in Interceptors:
            interceptor.enable()
    except ImportError as e:
        get_logger().debug(f"import httpx:{e}")
    except:
        get_logger().info(f"unknown error in httpx module")


__all__ = ['monkey_patch']