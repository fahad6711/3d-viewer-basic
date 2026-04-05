#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr float kInfinity = std::numeric_limits<float>::max();
constexpr float kPi = 3.14159265358979323846f;
constexpr float kBias = 1e-4f;

template <typename T>
class Vec3 {
public:
    T x;
    T y;
    T z;

    Vec3() : x(T(0)), y(T(0)), z(T(0)) {}
    explicit Vec3(T value) : x(value), y(value), z(value) {}
    Vec3(T xValue, T yValue, T zValue) : x(xValue), y(yValue), z(zValue) {}

    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator-() const { return Vec3(-x, -y, -z); }
    Vec3 operator*(T scale) const { return Vec3(x * scale, y * scale, z * scale); }
    Vec3 operator/(T scale) const { return Vec3(x / scale, y / scale, z / scale); }
    Vec3 operator*(const Vec3& other) const { return Vec3(x * other.x, y * other.y, z * other.z); }

    Vec3& operator+=(const Vec3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vec3& operator*=(T scale)
    {
        x *= scale;
        y *= scale;
        z *= scale;
        return *this;
    }

    T dot(const Vec3& other) const { return x * other.x + y * other.y + z * other.z; }
    T length2() const { return x * x + y * y + z * z; }
    T length() const { return std::sqrt(length2()); }

    Vec3& normalize()
    {
        const T len2 = length2();
        if (len2 > T(0)) {
            const T invLen = T(1) / std::sqrt(len2);
            x *= invLen;
            y *= invLen;
            z *= invLen;
        }
        return *this;
    }
};

template <typename T>
Vec3<T> operator*(T scale, const Vec3<T>& vec)
{
    return vec * scale;
}

using Vec3f = Vec3<float>;

float mix(float a, float b, float amount)
{
    return a * (1.0f - amount) + b * amount;
}

float clamp01(float value)
{
    return std::max(0.0f, std::min(1.0f, value));
}

bool approximatelyEqual(float left, float right, float epsilon = 1e-3f)
{
    return std::fabs(left - right) <= epsilon;
}

bool isFiniteVec(const Vec3f& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

float luminance(const Vec3f& color)
{
    return 0.2126f * color.x + 0.7152f * color.y + 0.0722f * color.z;
}

Vec3f clampColor(const Vec3f& color)
{
    return Vec3f(clamp01(color.x), clamp01(color.y), clamp01(color.z));
}

Vec3f backgroundColor(const Vec3f& rayDirection)
{
    const float t = clamp01(0.5f * (rayDirection.y + 1.0f));
    return Vec3f(0.18f, 0.24f, 0.35f) * (1.0f - t) + Vec3f(0.72f, 0.84f, 1.0f) * t;
}

class Sphere {
public:
    Vec3f center;
    float radius;
    float radius2;
    Vec3f surfaceColor;
    Vec3f emissionColor;
    float transparency;
    float reflection;

    Sphere(
        const Vec3f& centerValue,
        float radiusValue,
        const Vec3f& surface,
        float reflectionValue = 0.0f,
        float transparencyValue = 0.0f,
        const Vec3f& emission = Vec3f(0.0f))
        : center(centerValue),
          radius(radiusValue),
          radius2(radiusValue * radiusValue),
          surfaceColor(surface),
          emissionColor(emission),
          transparency(transparencyValue),
          reflection(reflectionValue)
    {
    }

    bool intersect(const Vec3f& rayOrigin, const Vec3f& rayDirection, float& t0, float& t1) const
    {
        const Vec3f offset = center - rayOrigin;
        const float projection = offset.dot(rayDirection);
        const float distance2 = offset.dot(offset) - projection * projection;
        if (distance2 > radius2) {
            return false;
        }

        const float thc = std::sqrt(radius2 - distance2);
        t0 = projection - thc;
        t1 = projection + thc;
        return t1 >= 0.0f;
    }
};

struct Scene {
    std::string name;
    std::vector<Sphere> spheres;
};

struct RenderConfig {
    int width = 120;
    int height = 68;
    int samples = 4;
    int maxDepth = 5;
    float fov = 30.0f;
    bool saveFile = true;
    bool preview = true;
    bool showProgress = true;
    std::string outputPath = "advanced_render.ppm";
    std::string sceneName = "classic";
};

struct RenderStats {
    double durationMs = 0.0;
    double averageLuminance = 0.0;
    std::uint64_t checksum = 0;
};

class Image {
public:
    Image(int widthValue, int heightValue) : width(widthValue), height(heightValue), pixels(widthValue * heightValue, Vec3f(0.0f)) {}

    Vec3f& at(int x, int y) { return pixels[static_cast<std::size_t>(y) * width + x]; }
    const Vec3f& at(int x, int y) const { return pixels[static_cast<std::size_t>(y) * width + x]; }

    double averageLuminance() const
    {
        double total = 0.0;
        for (const Vec3f& pixel : pixels) {
            total += clamp01(luminance(pixel));
        }
        return pixels.empty() ? 0.0 : total / static_cast<double>(pixels.size());
    }

    std::uint64_t checksum() const
    {
        std::uint64_t total = 0;
        for (const Vec3f& pixel : pixels) {
            const Vec3f clamped = clampColor(pixel);
            total += static_cast<std::uint64_t>(clamped.x * 255.0f);
            total += static_cast<std::uint64_t>(clamped.y * 255.0f);
            total += static_cast<std::uint64_t>(clamped.z * 255.0f);
        }
        return total;
    }

    bool writePPM(const std::string& path) const
    {
        std::ofstream stream(path, std::ios::binary);
        if (!stream) {
            return false;
        }

        stream << "P6\n" << width << ' ' << height << "\n255\n";
        for (const Vec3f& pixel : pixels) {
            const Vec3f clamped = clampColor(pixel);
            const unsigned char red = static_cast<unsigned char>(clamped.x * 255.0f);
            const unsigned char green = static_cast<unsigned char>(clamped.y * 255.0f);
            const unsigned char blue = static_cast<unsigned char>(clamped.z * 255.0f);
            stream.write(reinterpret_cast<const char*>(&red), 1);
            stream.write(reinterpret_cast<const char*>(&green), 1);
            stream.write(reinterpret_cast<const char*>(&blue), 1);
        }
        return static_cast<bool>(stream);
    }

    std::string asciiPreview(int columns = 72) const
    {
        static const std::string ramp = " .:-=+*#%@";
        if (width == 0 || height == 0) {
            return {};
        }

        const int previewWidth = std::min(columns, width);
        const double aspect = static_cast<double>(height) / static_cast<double>(width);
        const int previewHeight = std::max(1, static_cast<int>(std::round(previewWidth * aspect * 0.5)));

        std::ostringstream stream;
        for (int y = 0; y < previewHeight; ++y) {
            const int sourceY = std::min(height - 1, static_cast<int>(y * static_cast<double>(height) / previewHeight));
            for (int x = 0; x < previewWidth; ++x) {
                const int sourceX = std::min(width - 1, static_cast<int>(x * static_cast<double>(width) / previewWidth));
                const float light = clamp01(luminance(at(sourceX, sourceY)));
                const std::size_t index = static_cast<std::size_t>(light * (ramp.size() - 1));
                stream << ramp[index];
            }
            stream << '\n';
        }
        return stream.str();
    }

    int width;
    int height;
    std::vector<Vec3f> pixels;
};

Scene makeClassicScene()
{
    Scene scene;
    scene.name = "classic";
    scene.spheres.push_back(Sphere(Vec3f(0.0f, -10004.0f, -20.0f), 10000.0f, Vec3f(0.20f, 0.20f, 0.20f)));
    scene.spheres.push_back(Sphere(Vec3f(0.0f, 0.0f, -20.0f), 4.0f, Vec3f(1.00f, 0.32f, 0.36f), 1.0f, 0.5f));
    scene.spheres.push_back(Sphere(Vec3f(5.0f, -1.0f, -15.0f), 2.0f, Vec3f(0.90f, 0.76f, 0.46f), 1.0f));
    scene.spheres.push_back(Sphere(Vec3f(5.0f, 0.0f, -25.0f), 3.0f, Vec3f(0.65f, 0.77f, 0.97f), 1.0f));
    scene.spheres.push_back(Sphere(Vec3f(-5.5f, 0.0f, -15.0f), 3.0f, Vec3f(0.90f, 0.90f, 0.90f), 1.0f));
    scene.spheres.push_back(Sphere(Vec3f(0.0f, 20.0f, -30.0f), 3.0f, Vec3f(0.0f), 0.0f, 0.0f, Vec3f(3.0f)));
    return scene;
}

Scene makeSunsetScene()
{
    Scene scene;
    scene.name = "sunset";
    scene.spheres.push_back(Sphere(Vec3f(0.0f, -10003.0f, -18.0f), 10000.0f, Vec3f(0.28f, 0.18f, 0.14f)));
    scene.spheres.push_back(Sphere(Vec3f(-3.0f, -0.5f, -14.0f), 2.5f, Vec3f(0.95f, 0.42f, 0.25f), 0.55f));
    scene.spheres.push_back(Sphere(Vec3f(2.5f, 1.0f, -18.0f), 3.0f, Vec3f(0.30f, 0.50f, 0.95f), 0.9f, 0.1f));
    scene.spheres.push_back(Sphere(Vec3f(5.5f, -1.0f, -12.0f), 1.8f, Vec3f(0.82f, 0.74f, 0.35f), 0.35f));
    scene.spheres.push_back(Sphere(Vec3f(-2.0f, 8.0f, -12.0f), 1.5f, Vec3f(0.0f), 0.0f, 0.0f, Vec3f(3.6f, 2.1f, 1.2f)));
    scene.spheres.push_back(Sphere(Vec3f(6.5f, 10.0f, -25.0f), 2.0f, Vec3f(0.0f), 0.0f, 0.0f, Vec3f(1.2f, 1.5f, 3.0f)));
    return scene;
}

Scene makeDiagnosticScene()
{
    Scene scene;
    scene.name = "diagnostic";
    scene.spheres.push_back(Sphere(Vec3f(0.0f, -10002.0f, -16.0f), 10000.0f, Vec3f(0.18f, 0.18f, 0.18f)));
    scene.spheres.push_back(Sphere(Vec3f(0.0f, 0.0f, -12.0f), 2.4f, Vec3f(0.25f, 0.80f, 0.40f), 0.4f));
    scene.spheres.push_back(Sphere(Vec3f(-3.6f, 0.2f, -10.0f), 1.7f, Vec3f(0.86f, 0.30f, 0.35f), 0.6f));
    scene.spheres.push_back(Sphere(Vec3f(3.8f, 1.0f, -14.0f), 2.0f, Vec3f(0.32f, 0.58f, 0.92f), 0.9f));
    scene.spheres.push_back(Sphere(Vec3f(0.0f, 7.0f, -10.0f), 1.3f, Vec3f(0.0f), 0.0f, 0.0f, Vec3f(2.8f)));
    return scene;
}

Scene buildScene(const std::string& name)
{
    if (name == "classic") {
        return makeClassicScene();
    }
    if (name == "sunset") {
        return makeSunsetScene();
    }
    if (name == "diagnostic") {
        return makeDiagnosticScene();
    }
    throw std::runtime_error("Unknown scene '" + name + "'. Available scenes: classic, sunset, diagnostic");
}

Vec3f trace(const Vec3f& rayOrigin, const Vec3f& rayDirection, const Scene& scene, int depth, int maxDepth)
{
    float nearest = kInfinity;
    const Sphere* hitSphere = nullptr;

    for (const Sphere& sphere : scene.spheres) {
        float t0 = kInfinity;
        float t1 = kInfinity;
        if (!sphere.intersect(rayOrigin, rayDirection, t0, t1)) {
            continue;
        }
        if (t0 < 0.0f) {
            t0 = t1;
        }
        if (t0 < nearest) {
            nearest = t0;
            hitSphere = &sphere;
        }
    }

    if (hitSphere == nullptr) {
        return backgroundColor(rayDirection);
    }

    const Vec3f hitPoint = rayOrigin + rayDirection * nearest;
    Vec3f hitNormal = hitPoint - hitSphere->center;
    hitNormal.normalize();

    bool inside = false;
    if (rayDirection.dot(hitNormal) > 0.0f) {
        hitNormal = -hitNormal;
        inside = true;
    }

    Vec3f surfaceColor = hitSphere->surfaceColor * 0.05f;
    if ((hitSphere->transparency > 0.0f || hitSphere->reflection > 0.0f) && depth < maxDepth) {
        const float facingRatio = std::max(0.0f, -rayDirection.dot(hitNormal));
        const float fresnelEffect = mix(std::pow(1.0f - facingRatio, 3.0f), 1.0f, 0.1f);

        Vec3f reflectionDirection = rayDirection - hitNormal * 2.0f * rayDirection.dot(hitNormal);
        reflectionDirection.normalize();
        const Vec3f reflection = trace(hitPoint + hitNormal * kBias, reflectionDirection, scene, depth + 1, maxDepth);

        Vec3f refraction(0.0f);
        if (hitSphere->transparency > 0.0f) {
            const float ior = 1.1f;
            const float eta = inside ? ior : (1.0f / ior);
            const float cosi = -hitNormal.dot(rayDirection);
            const float k = 1.0f - eta * eta * (1.0f - cosi * cosi);
            if (k >= 0.0f) {
                Vec3f refractionDirection = rayDirection * eta + hitNormal * (eta * cosi - std::sqrt(k));
                refractionDirection.normalize();
                refraction = trace(hitPoint - hitNormal * kBias, refractionDirection, scene, depth + 1, maxDepth);
            }
        }

        surfaceColor += (reflection * fresnelEffect + refraction * (1.0f - fresnelEffect) * hitSphere->transparency) *
            hitSphere->surfaceColor;
    } else {
        for (const Sphere& light : scene.spheres) {
            if (light.emissionColor.length2() <= 0.0f) {
                continue;
            }

            Vec3f lightDirection = light.center - hitPoint;
            const float lightDistance2 = lightDirection.length2();
            lightDirection.normalize();

            bool blocked = false;
            for (const Sphere& obstacle : scene.spheres) {
                if (&obstacle == &light) {
                    continue;
                }

                float t0 = kInfinity;
                float t1 = kInfinity;
                if (!obstacle.intersect(hitPoint + hitNormal * kBias, lightDirection, t0, t1)) {
                    continue;
                }
                if (t0 < 0.0f) {
                    t0 = t1;
                }
                if (t0 > 0.0f && t0 * t0 < lightDistance2) {
                    blocked = true;
                    break;
                }
            }

            if (!blocked) {
                const float intensity = std::max(0.0f, hitNormal.dot(lightDirection));
                const float attenuation = 1.0f / (1.0f + 0.0005f * lightDistance2);
                surfaceColor += hitSphere->surfaceColor * intensity * attenuation * light.emissionColor;
            }
        }
    }

    return surfaceColor + hitSphere->emissionColor;
}

Image renderScene(const Scene& scene, const RenderConfig& config, RenderStats& stats)
{
    if (config.width <= 0 || config.height <= 0 || config.samples <= 0 || config.maxDepth <= 0) {
        throw std::runtime_error("Width, height, samples, and depth must all be positive integers.");
    }

    Image image(config.width, config.height);
    const auto start = std::chrono::steady_clock::now();

    const float inverseWidth = 1.0f / static_cast<float>(config.width);
    const float inverseHeight = 1.0f / static_cast<float>(config.height);
    const float aspectRatio = static_cast<float>(config.width) / static_cast<float>(config.height);
    const float angle = std::tan(kPi * 0.5f * config.fov / 180.0f);
    const int sampleGrid = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<float>(config.samples)))));

    int nextProgress = 0;
    for (int y = 0; y < config.height; ++y) {
        if (config.showProgress) {
            const int progress = static_cast<int>(100.0 * y / std::max(1, config.height - 1));
            if (progress >= nextProgress) {
                std::cout << "\rRendering scene '" << scene.name << "' ... " << std::setw(3) << progress << '%' << std::flush;
                nextProgress += 10;
            }
        }

        for (int x = 0; x < config.width; ++x) {
            Vec3f color(0.0f);
            for (int sample = 0; sample < config.samples; ++sample) {
                const int sampleX = sample % sampleGrid;
                const int sampleY = sample / sampleGrid;
                const float jitterX = (static_cast<float>(sampleX) + 0.5f) / sampleGrid;
                const float jitterY = (static_cast<float>(sampleY) + 0.5f) / sampleGrid;
                const float rayX = (2.0f * ((x + jitterX) * inverseWidth) - 1.0f) * angle * aspectRatio;
                const float rayY = (1.0f - 2.0f * ((y + jitterY) * inverseHeight)) * angle;

                Vec3f rayDirection(rayX, rayY, -1.0f);
                rayDirection.normalize();
                color += trace(Vec3f(0.0f), rayDirection, scene, 0, config.maxDepth);
            }

            image.at(x, y) = color / static_cast<float>(config.samples);
        }
    }

    if (config.showProgress) {
        std::cout << "\rRendering scene '" << scene.name << "' ... 100%\n";
    }

    const auto finish = std::chrono::steady_clock::now();
    stats.durationMs = std::chrono::duration<double, std::milli>(finish - start).count();
    stats.averageLuminance = image.averageLuminance();
    stats.checksum = image.checksum();
    return image;
}

void printUsage(const char* program)
{
    std::cout
        << "Advanced terminal ray tracer\n"
        << "Usage: " << program << " [options]\n\n"
        << "Options:\n"
        << "  --width N        Output width in pixels\n"
        << "  --height N       Output height in pixels\n"
        << "  --samples N      Samples per pixel\n"
        << "  --depth N        Maximum ray recursion depth\n"
        << "  --scene NAME     Scene preset: classic, sunset, diagnostic\n"
        << "  --output PATH    Save binary PPM image to PATH\n"
        << "  --no-file        Skip writing the PPM image\n"
        << "  --no-preview     Skip ASCII terminal preview\n"
        << "  --no-progress    Skip progress updates\n"
        << "  --self-test      Run white-box internal tests\n"
        << "  --help           Show this help message\n";
}

int parsePositiveInt(const std::string& option, const char* value)
{
    const int parsed = std::stoi(value);
    if (parsed <= 0) {
        throw std::runtime_error(option + " must be a positive integer.");
    }
    return parsed;
}

struct CommandLine {
    RenderConfig config;
    bool help = false;
    bool selfTest = false;
};

CommandLine parseArguments(int argc, char** argv)
{
    CommandLine commandLine;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help") {
            commandLine.help = true;
            continue;
        }
        if (argument == "--self-test") {
            commandLine.selfTest = true;
            continue;
        }
        if (argument == "--no-file") {
            commandLine.config.saveFile = false;
            continue;
        }
        if (argument == "--no-preview") {
            commandLine.config.preview = false;
            continue;
        }
        if (argument == "--no-progress") {
            commandLine.config.showProgress = false;
            continue;
        }
        if (index + 1 >= argc) {
            throw std::runtime_error("Missing value for " + argument);
        }

        const char* value = argv[++index];
        if (argument == "--width") {
            commandLine.config.width = parsePositiveInt(argument, value);
        } else if (argument == "--height") {
            commandLine.config.height = parsePositiveInt(argument, value);
        } else if (argument == "--samples") {
            commandLine.config.samples = parsePositiveInt(argument, value);
        } else if (argument == "--depth") {
            commandLine.config.maxDepth = parsePositiveInt(argument, value);
        } else if (argument == "--scene") {
            commandLine.config.sceneName = value;
        } else if (argument == "--output") {
            commandLine.config.outputPath = value;
        } else {
            throw std::runtime_error("Unknown argument: " + argument);
        }
    }

    return commandLine;
}

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void testVectorNormalization()
{
    Vec3f vector(3.0f, 4.0f, 0.0f);
    vector.normalize();
    expect(approximatelyEqual(vector.length(), 1.0f, 1e-4f), "normalize() should produce a unit vector");
}

void testSphereIntersection()
{
    Sphere sphere(Vec3f(0.0f, 0.0f, -5.0f), 1.0f, Vec3f(1.0f));
    float t0 = 0.0f;
    float t1 = 0.0f;
    const bool hit = sphere.intersect(Vec3f(0.0f), Vec3f(0.0f, 0.0f, -1.0f), t0, t1);
    expect(hit, "Expected center ray to intersect sphere");
    expect(approximatelyEqual(t0, 4.0f, 1e-3f), "Nearest hit distance should be approximately 4");
    expect(approximatelyEqual(t1, 6.0f, 1e-3f), "Far hit distance should be approximately 6");
}

void testSphereMiss()
{
    Sphere sphere(Vec3f(0.0f, 0.0f, -5.0f), 1.0f, Vec3f(1.0f));
    float t0 = 0.0f;
    float t1 = 0.0f;
    const bool hit = sphere.intersect(Vec3f(0.0f), Vec3f(0.0f, 1.0f, 0.0f), t0, t1);
    expect(!hit, "Expected upward ray to miss the sphere");
}

void testBackgroundGradient()
{
    const Vec3f upward = backgroundColor(Vec3f(0.0f, 1.0f, 0.0f));
    const Vec3f downward = backgroundColor(Vec3f(0.0f, -1.0f, 0.0f));
    expect(luminance(upward) > luminance(downward), "Sky should be brighter overhead than below");
}

void testTraceAndMiniRender()
{
    const Scene scene = makeDiagnosticScene();
    Vec3f rayDirection(0.0f, 0.0f, -1.0f);
    rayDirection.normalize();

    const Vec3f traced = trace(Vec3f(0.0f), rayDirection, scene, 0, 4);
    expect(isFiniteVec(traced), "trace() should return finite values");
    expect(luminance(traced) > 0.02f, "Center ray should hit illuminated geometry");

    RenderConfig config;
    config.width = 24;
    config.height = 14;
    config.samples = 1;
    config.maxDepth = 3;
    config.preview = false;
    config.showProgress = false;
    config.saveFile = false;
    config.sceneName = "diagnostic";

    RenderStats stats;
    const Image image = renderScene(scene, config, stats);
    expect(image.width == 24 && image.height == 14, "Mini render should preserve configured dimensions");
    expect(stats.averageLuminance > 0.01 && stats.averageLuminance < 1.0, "Mini render average luminance should be in range");
    expect(stats.checksum > 0, "Mini render checksum should be non-zero");
}

void runSelfTests()
{
    std::cout << "Running white-box tests...\n";
    testVectorNormalization();
    std::cout << "  [PASS] Vector normalization\n";
    testSphereIntersection();
    std::cout << "  [PASS] Sphere intersection\n";
    testSphereMiss();
    std::cout << "  [PASS] Sphere miss handling\n";
    testBackgroundGradient();
    std::cout << "  [PASS] Background gradient\n";
    testTraceAndMiniRender();
    std::cout << "  [PASS] Trace and mini render invariants\n";
    std::cout << "WHITE-BOX TESTS PASSED\n";
}

void printRenderSummary(const Scene& scene, const RenderConfig& config, const RenderStats& stats, const Image& image)
{
    std::cout << "Render complete\n";
    std::cout << "  Scene: " << scene.name << '\n';
    std::cout << "  Resolution: " << config.width << "x" << config.height << '\n';
    std::cout << "  Samples: " << config.samples << '\n';
    std::cout << "  Max depth: " << config.maxDepth << '\n';
    std::cout << "  Spheres: " << scene.spheres.size() << '\n';
    std::cout << "  Average luminance: " << std::fixed << std::setprecision(4) << stats.averageLuminance << '\n';
    std::cout << "  Checksum: " << stats.checksum << '\n';
    std::cout << "  Duration: " << std::setprecision(2) << stats.durationMs << " ms\n";
    std::cout << "  Center pixel luminance: "
              << std::setprecision(4)
              << luminance(image.at(config.width / 2, config.height / 2))
              << '\n';
}

} 

int main(int argc, char** argv)
{
    try {
        const CommandLine commandLine = parseArguments(argc, argv);
        if (commandLine.help) {
            printUsage(argv[0]);
            return 0;
        }

        if (commandLine.selfTest) {
            runSelfTests();
            return 0;
        }

        const Scene scene = buildScene(commandLine.config.sceneName);
        RenderStats stats;
        const Image image = renderScene(scene, commandLine.config, stats);

        printRenderSummary(scene, commandLine.config, stats, image);

        if (commandLine.config.preview) {
            std::cout << "\nASCII PREVIEW\n";
            std::cout << image.asciiPreview() << '\n';
        }

        if (commandLine.config.saveFile) {
            if (!image.writePPM(commandLine.config.outputPath)) {
                throw std::runtime_error("Failed to write image to '" + commandLine.config.outputPath + "'.");
            }
            std::cout << "Saved image to " << commandLine.config.outputPath << '\n';
        } else {
            std::cout << "File output disabled\n";
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
