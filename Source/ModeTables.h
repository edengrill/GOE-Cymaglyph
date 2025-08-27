#pragma once

#include <vector>
#include <algorithm>
#include <cmath>

namespace CymaglyphModes {

// Bessel function zeros for J_n(alpha_{n,k} * r) = 0
static constexpr double J0_zeros[] = {2.4048255577, 5.5200781103, 8.6537279129, 11.7915344391, 14.9309177086};
static constexpr double J1_zeros[] = {3.8317059702, 7.0155866698, 10.1734681351, 13.3236919363, 16.4706300509};
static constexpr double J2_zeros[] = {5.1356223018, 8.4172441404, 11.6198411721, 14.7959517824, 17.9598194950};
static constexpr double J3_zeros[] = {6.3801618952, 9.7610231299, 13.0152007217, 16.2234660112, 19.4094152264};
static constexpr double J4_zeros[] = {7.5883424345, 11.0647094885, 14.3725366716, 17.6159660498, 20.8269329569};

struct CircularMode {
    int n;        // angular mode number
    int k;        // radial mode number  
    double alpha; // Bessel zero value
    
    bool operator<(const CircularMode& other) const {
        return alpha < other.alpha;
    }
};

struct SquareMode {
    int m;
    int n;
    double lambda; // sqrt(m^2 + n^2)
    
    bool operator<(const SquareMode& other) const {
        return lambda < other.lambda;
    }
};

// Build sorted mode lists
inline std::vector<CircularMode> getCircularModes() {
    std::vector<CircularMode> modes;
    
    // n=0 modes
    for (int k = 0; k < 5; k++)
        modes.push_back({0, k+1, J0_zeros[k]});
    
    // n=1 modes
    for (int k = 0; k < 5; k++)
        modes.push_back({1, k+1, J1_zeros[k]});
        
    // n=2 modes
    for (int k = 0; k < 5; k++)
        modes.push_back({2, k+1, J2_zeros[k]});
        
    // n=3 modes
    for (int k = 0; k < 5; k++)
        modes.push_back({3, k+1, J3_zeros[k]});
    
    // n=4 modes
    for (int k = 0; k < 5; k++)
        modes.push_back({4, k+1, J4_zeros[k]});
    
    std::sort(modes.begin(), modes.end());
    return modes;
}

inline std::vector<SquareMode> getSquareModes() {
    std::vector<SquareMode> modes;
    
    // Generate modes up to (8,8)
    for (int m = 1; m <= 8; m++) {
        for (int n = 1; n <= 8; n++) {
            double lambda = std::sqrt(m*m + n*n);
            modes.push_back({m, n, lambda});
        }
    }
    
    std::sort(modes.begin(), modes.end());
    return modes;
}

// Map frequency to mode rank (0.0 to 1.0 normalized)
inline float frequencyToModeRank(float freqHz, float minFreq = 27.5f, float maxFreq = 3520.0f) {
    float logFreq = std::log(freqHz);
    float logMin = std::log(minFreq);
    float logMax = std::log(maxFreq);
    return std::clamp((logFreq - logMin) / (logMax - logMin), 0.0f, 1.0f);
}

// Get two modes to crossfade between based on rank
template<typename ModeType>
inline std::pair<ModeType, ModeType> getModePair(const std::vector<ModeType>& modes, float rank) {
    if (modes.empty()) return {};
    
    float modeIndex = rank * (modes.size() - 1);
    int lower = static_cast<int>(std::floor(modeIndex));
    int upper = std::min(lower + 1, static_cast<int>(modes.size() - 1));
    
    return {modes[lower], modes[upper]};
}

// Get crossfade fraction between modes
inline float getModeCrossfade(float rank, int numModes) {
    float modeIndex = rank * (numModes - 1);
    return std::fmod(modeIndex, 1.0f);
}

// Center clamping weight calculation
inline float getCenterClampWeight(int m, int n, bool isCenterClamped) {
    if (!isCenterClamped) return 1.0f;
    
    // For square modes, check if mode has antinode at center (0.5, 0.5)
    bool mOdd = (m % 2 == 1);
    bool nOdd = (n % 2 == 1);
    
    // Both odd = antinode at center, suppress
    if (mOdd && nOdd) return 0.1f;
    
    return 1.0f;
}

// Cheap Bessel J approximation for shader use (first few terms)
inline float besselJ0Approx(float x) {
    float x2 = x * x;
    return 1.0f - x2/4.0f + x2*x2/64.0f - x2*x2*x2/2304.0f;
}

inline float besselJ1Approx(float x) {
    float x2 = x * x;
    return x/2.0f * (1.0f - x2/8.0f + x2*x2/192.0f);
}

inline float besselJ2Approx(float x) {
    float x2 = x * x;
    return x2/8.0f * (1.0f - x2/12.0f + x2*x2/384.0f);
}

inline float besselJ3Approx(float x) {
    float x2 = x * x;
    float x3 = x2 * x;
    return x3/48.0f * (1.0f - x2/16.0f);
}

// Water/Faraday mode parameters
struct WaterMode {
    int n;       // angular symmetry
    float k1;    // first radial wavenumber
    float k2;    // second radial wavenumber
    float amp1;  // first component amplitude
    float amp2;  // second component amplitude
};

inline WaterMode getWaterMode(float freqHz) {
    // Map frequency to interesting visual patterns
    float fNorm = std::log2(freqHz / 100.0f);
    int n = 2 + static_cast<int>(std::fmod(fNorm * 2.0f, 6.0f));
    
    float k1 = 4.0f + std::fmod(fNorm * 3.0f, 8.0f);
    float k2 = k1 * 1.618f; // golden ratio for visual interest
    
    return {n, k1, k2, 1.0f, 0.6f};
}

} // namespace CymaglyphModes