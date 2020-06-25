// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#pragma once

#include "image.hpp"

/**
 * @brief Load image from file.
 *
 * @param[in] file path to the file to load
 *
 * @throw std::system_error if file operation fails
 * @throw std::runtime_error on format error
 *
 * @return image instance
 */
image load_image(const char* file);

/**
 * @brief Print list of supported formats.
 */
void print_formats();
