// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include <cstdint>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "RealSenseID/Preview.h"


namespace py = pybind11;

using ImageCallbackFun = std::function<void(RealSenseID::Image)>;

// Callback support using virtual methods
class PreviewImageCallbackPy : public RealSenseID::PreviewImageReadyCallback {
	ImageCallbackFun _user_clbk;
public:
	explicit PreviewImageCallbackPy(ImageCallbackFun& user_clbk) :_user_clbk{ user_clbk } {}
	void OnPreviewImageReady(const RealSenseID::Image image) override {
		py::gil_scoped_acquire acquire;
		_user_clbk(image);
	}
};

// store preview and callback as unique pointers, so that the callback won't get deleted too soon
class PreviewPy {
	std::unique_ptr<PreviewImageCallbackPy> _img_clbk;
	std::unique_ptr<RealSenseID::Preview> _preview;

public:
	PreviewPy(RealSenseID::PreviewConfig config)
	{
		_preview = std::make_unique<RealSenseID::Preview>(config);
	}

	void Start(ImageCallbackFun fn)
	{
		Stop();
		_img_clbk = std::make_unique< PreviewImageCallbackPy>(fn);
		if (!_preview->StartPreview(*_img_clbk))
			throw std::runtime_error("StartPreview failed");
	}

	void Stop()
	{
		if (!_preview->StopPreview())
			throw std::runtime_error("StopPreview failed");
	}
};



void init_preview(pybind11::module& m)
{
	using RealSenseID::PreviewMode;
	using RealSenseID::PreviewConfig;
	using RealSenseID::ImageMetadata;
	using RealSenseID::Image;

	py::enum_<PreviewMode>(m, "PreviewMode")
        .value("MJPEG_1080P", PreviewMode::MJPEG_1080P)
		.value("MJPEG_720P", PreviewMode::MJPEG_720P)
		.value("RAW10_1080P", PreviewMode::RAW10_1080P);

	py::class_<PreviewConfig>(m, "PreviewConfig")
		.def(py::init<>())
		.def_readwrite("camera_number", &PreviewConfig::cameraNumber)
		.def_readwrite("preview_mode", &PreviewConfig::previewMode);

	py::class_<ImageMetadata>(m, "ImageMetadata")
		.def(py::init<>())
		.def_readonly("timestamp", &ImageMetadata::timestamp)
		.def_readonly("status", &ImageMetadata::status)
		.def_readonly("sensor_id", &ImageMetadata::sensor_id)
		.def_readonly("led", &ImageMetadata::led)
		.def_readonly("projector", &ImageMetadata::projector);

	py::class_<Image>(m, "Image")
		.def(py::init<>())
		/*.def_readonly("buffer", &Image::buffer)*/
		.def_readonly("size", &Image::size)
		.def_readonly("width", &Image::width)
		.def_readonly("height", &Image::height)
		.def_readonly("stride", &Image::stride)
		.def_readonly("number", &Image::number)
		.def_readonly("metadata", &Image::metadata)
		.def("get_buffer", [](const Image& self) {
		return py::memoryview::from_memory(
			self.buffer,                // buffer pointer
			sizeof(uint8_t) * self.size // buffer size
		);
	});


	py::class_<PreviewPy>(m, "Preview")
		.def(py::init<const PreviewConfig&>())
		.def("start", &PreviewPy::Start,
			py::call_guard<py::gil_scoped_release>())
		.def("stop", &PreviewPy::Stop,
			py::call_guard<py::gil_scoped_release>());
}