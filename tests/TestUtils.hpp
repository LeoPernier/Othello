// tests/TestUtils.hpp

#pragma once

#include <iostream>
#include <functional>
#include <string>
#include <sstream>
#include <iomanip>
#include<chrono>

static constexpr int kResultColumn   = 60;
static constexpr int kTimeFieldWidth = 8;
static constexpr int kIndexWidth     = 2;

static constexpr const char* kColorGreen = "\033[32m";
static constexpr const char* kColorRed   = "\033[31m";
static constexpr const char* kColorReset = "\033[0m";
static constexpr const char* kColorBold  = "\033[1m";

static int g_passes    = 0;
static int g_failures  = 0;
static int g_testIndex = 0;

/**
 * \brief    Formate une durée exprimée en nanosecondes en chaîne lisible (ns, μs, ms, s)
 * \return   true si le test/résultat est conforme, false sinon.
 */
static std::string formatTime(long long ns) {
    std::ostringstream oss;
    if (ns >= 10'000'000'000LL)
        oss << std::fixed << std::setprecision(3) << (ns / 1e9) << " s";
    else if (ns >= 10'000'000LL)
        oss << (ns / 1'000'000) << " ms";
    else if (ns >= 10'000LL)
        oss << (ns / 1'000) << " μs";
    else
        oss << ns << " ns";
    return oss.str();
}

/**
 * \brief Affiche une en-tête de section pour grouper une série de tests.
*/
inline void startSection(const std::string& name) {
    std::cout << "\n" << kColorBold << "=== " << name << " ===" << kColorReset << "\n\n";
}

/**
 * \brief Exécute un test, mesure le temps d’exécution et affiche « OK »/« FAIL » avec couleur.
 */
void runTest(const std::string& name, std::function<bool()> fn) {
    int idx = ++g_testIndex;

    std::ostringstream prefix;
    prefix << "Test " << std::setw(kIndexWidth) << std::setfill(' ') << idx << ": " << name << " ";
    std::string p = prefix.str();

    int dashCount = kResultColumn - int(p.size());
    if (dashCount < 1)
        dashCount = 1;

    auto      t0 = std::chrono::high_resolution_clock::now();
    bool      ok = fn();
    auto      t1 = std::chrono::high_resolution_clock::now();
    long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    std::string ft   = formatTime(ns);
    auto        pos  = ft.find_last_of(' ');
    std::string val  = ft.substr(0, pos);
    std::string unit = ft.substr(pos + 1);

    if (ok)
        ++g_passes;
    else
        ++g_failures;
    const char* color = ok ? kColorGreen : kColorRed;
    const char* label = ok ? " PASS" : " FAIL";

    int pad = kTimeFieldWidth - int(val.size());
    if (pad < 1)
        pad = 1;

    std::cout << p << std::string(dashCount, '-') << color << label << kColorReset << " (" << val << std::string(pad, ' ') << unit << ")\n";
}

/**
 * \brief  Affiche le récapitulatif global et renvoie le code de retour (0 si tous OK)
 * \return true si le test/résultat est conforme, false sinon.
 */
int printSummaryAndReturn() {
    int total = g_passes + g_failures;
    std::cout << "\nTests run: " << total
              << "  Passed: "    << g_passes
              << "  Failed: "    << g_failures << "\n";
    return (g_failures == 0 ? 0 : 1);
}
