/*
    src/exponential.h -- Exponential tonemapping operator
    
    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license. 
*/

#pragma once

#include <tonemap.h>

class ExponentialOperator : public TonemapOperator {
public:
	ExponentialOperator() : TonemapOperator() {
		parameters["Gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value");
		parameters["p"] = Parameter(1.f, 0.f, 20.f, "p", "Exponent numerator scale factor");
		parameters["q"] = Parameter(1.f, 0.f, 20.f, "q", "Exponent denominator scale factor");

		name = "Exponential";
		description = "Exponential Mapping\n\nProposed in \"A Comparison of techniques for the Transformation of Radiosity Values to Monitor Colors\" by Ferschin et al. 1994.";

		shader->init(
			"Exponential",

			"#version 330\n"
			"in vec2 position;\n"
			"out vec2 uv;\n"
			"void main() {\n"
			"    gl_Position = vec4(position.x*2-1, position.y*2-1, 0.0, 1.0);\n"
			"    uv = vec2(position.x, 1-position.y);\n"
			"}",

			"#version 330\n"
			"uniform sampler2D source;\n"
			"uniform float exposure;\n"
			"uniform float gamma;\n"
			"uniform float Lavg;\n"
			"uniform float p;\n"
			"uniform float q;\n"
			"in vec2 uv;\n"
			"out vec4 out_color;\n"
			"\n"
			"vec4 clampedValue(vec4 color) {\n"
			"	 color.a = 1.0;\n"
			"	 return clamp(color, 0.0, 1.0);\n"
			"}\n"
			"\n"
			"vec4 gammaCorrect(vec4 color) {\n"
			"	 return pow(color, vec4(1.0/gamma));\n"
			"}\n"
			"\n"
			"float getLuminance(vec4 color) {\n"
			"	 return 0.212671 * color.r + 0.71516 * color.g + 0.072169 * color.b;\n"
			"}\n"
			"\n"
			"vec4 adjustColor(vec4 color, float L, float Ld) {\n"
			"	return Ld * color / L;\n"
			"}\n"
			"\n"
			"void main() {\n"
			"    vec4 color = exposure * texture(source, uv);\n"
			"	 float L = getLuminance(color);\n"
			"	 float Ld = 1.0 - exp(-(L * p) / (exposure * Lavg * q));\n"
			"	 color = adjustColor(color, L, Ld);\n"
			"	 color = clampedValue(color);\n"
			"    out_color = gammaCorrect(color);\n"
			"}"
		);
	}

	virtual void setParameters(const Image *image) override {
		parameters["Lavg"] = Parameter(image->getAverageLuminance(), "Lavg");
	};

	void process(const Image *image, uint8_t *dst, float exposure, float *progress) const override {
		const nanogui::Vector2i &size = image->getSize();
		*progress = 0.f;
		float delta = 1.f / (size.x() * size.y());

		float gamma = parameters.at("Gamma").value;
		float Lavg = parameters.at("Lavg").value;
		float p = parameters.at("p").value;
		float q = parameters.at("q").value;

		for (int i = 0; i < size.y(); ++i) {
			for (int j = 0; j < size.x(); ++j) {
				const Color3f &color = image->ref(i, j);
				float Lw = color.getLuminance();
				float Ld = map(Lw, exposure, Lavg, p, q);
				Color3f c = Ld * color / Lw;
				c = c.clampedValue();
				c = c.gammaCorrect(gamma);
				dst[0] = (uint8_t) (255.f * c.r());
				dst[1] = (uint8_t) (255.f * c.g());
				dst[2] = (uint8_t) (255.f * c.b());
				dst += 3;
				*progress += delta;
			}
		}
	}

	float graph(float value) const override {
		float gamma = parameters.at("Gamma").value;
		float Lavg = parameters.at("Lavg").value;
		float p = parameters.at("p").value;
		float q = parameters.at("q").value;

		value = map(value, 1.f, Lavg, p, q);
		value = clamp(value, 0.f, 1.f);
		value = std::pow(value, 1.f / gamma);
		return value;
	}

protected:
	float map(float Lw, float exposure, float Lavg, float p, float q) const {
		float L = exposure * Lw;
		float Ld = 1.f - std::exp(-(L * p) / (exposure * Lavg * q));
		return Ld;
	}
};