#include "core/SimulationEngine.h"

#include "utils/RandomGenerator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace bdss::core {

SimulationEngine::SimulationEngine(Config config)
    : config_(config),
      canteen_(config.tableRows, config.tableCols,
               config.enableCleaning ? config.cleaningTime : 0,
               config.enableSeatPreference ? config.windowProximityWeight : 0.0,
               config.enableSeatPreference ? config.groupProximityWeight : 0.0,
               config.enableSeatPreference ? config.isolationWeight : 0.0) {
    config_.validate();
    reset();
}

void SimulationEngine::reset() {
    config_.validate();
    currentTime_ = 0;
    nextStudentId_ = 1;
    canteen_ = Canteen(config_.tableRows, config_.tableCols,
                        config_.enableCleaning ? config_.cleaningTime : 0,
                        config_.enableSeatPreference ? config_.windowProximityWeight : 0.0,
                        config_.enableSeatPreference ? config_.groupProximityWeight : 0.0,
                        config_.enableSeatPreference ? config_.isolationWeight : 0.0);
    windows_.clear();
    const auto efficiencies = buildWindowEfficiencies();
    for (int i = 0; i < config_.windowCount; ++i) {
        windows_.emplace_back(i + 1, efficiencies.at(static_cast<std::size_t>(i)));
    }
    // 如果用户配置了窗口品类名称，为每个窗口赋品类
    if (!config_.windowCategories.empty()) {
        for (int i = 0; i < config_.windowCount && i < static_cast<int>(config_.windowCategories.size()); ++i) {
            windows_[static_cast<std::size_t>(i)].setCategory(config_.windowCategories[static_cast<std::size_t>(i)]);
        }
    }
    waitingForSeat_.clear();
    allStudents_.clear();
    finishedStudents_.clear();
    droppedStudents_ = 0;
    finalized_ = false;
    statistics_.clear();
    utils::RandomGenerator::setSeed(config_.randomSeed);
}

std::vector<double> SimulationEngine::buildWindowEfficiencies() const {
    std::vector<double> efficiencies;
    efficiencies.reserve(static_cast<std::size_t>(config_.windowCount));
    for (int i = 0; i < config_.windowCount; ++i) {
        if (config_.windowEfficiency == WindowEfficiency::Equal) {
            efficiencies.push_back(1.0);
        } else {
            // Deterministic variance around 1.0 avoids flaky tests and makes
            // capacity differences visible in simulation results.
            static constexpr double pattern[] = {0.85, 0.95, 1.05, 1.15, 1.00};
            efficiencies.push_back(pattern[static_cast<std::size_t>(i % 5)]);
        }
    }
    return efficiencies;
}

int SimulationEngine::boundedServiceTimeForWindow(int rawServiceTime, const Window& window) const noexcept {
    const auto adjusted = static_cast<int>(std::ceil(static_cast<double>(rawServiceTime) / window.getEfficiency()));
    return std::max(1, adjusted);
}

Window& SimulationEngine::chooseWindow() {
    return *std::min_element(windows_.begin(), windows_.end(), [](const Window& lhs, const Window& rhs) {
        if (lhs.getWorkloadLength() == rhs.getWorkloadLength()) {
            return lhs.getId() < rhs.getId();
        }
        return lhs.getWorkloadLength() < rhs.getWorkloadLength();
    });
}

Window& SimulationEngine::chooseWindowWithPreference(const std::shared_ptr<Student>& student) {
    // 偏好评分：score = queueLengthWeight * workload - (命中偏好品类 ? preferenceBonus : 0)
    // 取最小分窗口
    const int prefCat = student->getPreferredCategory();
    const double qw = config_.queueLengthWeight;
    const double pb = config_.preferenceBonus;

    auto best = windows_.begin();
    double bestScore = std::numeric_limits<double>::max();

    for (auto it = windows_.begin(); it != windows_.end(); ++it) {
        double score = qw * static_cast<double>(it->getWorkloadLength());
        // 如果学生有偏好品类且此窗口匹配，给予偏好加分（等效队列长度减短）
        if (prefCat >= 0 && !it->getCategory().empty() &&
            static_cast<std::size_t>(prefCat) < config_.windowCategories.size()) {
            // 品类名称匹配
            if (it->getCategory() == config_.windowCategories[static_cast<std::size_t>(prefCat)]) {
                score -= pb;
            }
        }
        if (score < bestScore ||
            (score == bestScore && it->getId() < best->getId())) {
            bestScore = score;
            best = it;
        }
    }
    return *best;
}

std::shared_ptr<Student> SimulationEngine::createStudentWithPreferences(int serviceTime, int diningTime) {
    const std::vector<double> ratios = {config_.ratioUndergrad, config_.ratioGrad, config_.ratioStaff};
    const auto profile = static_cast<ProfileType>(utils::RandomGenerator::getWeightedChoice(ratios));

    double diningFactor = 1.0;
    if (profile == ProfileType::Undergrad) diningFactor = config_.undergradDiningFactor;
    else if (profile == ProfileType::Staff) diningFactor = config_.staffDiningFactor;

    auto student = std::make_shared<Student>(nextStudentId_++, currentTime_, serviceTime,
        static_cast<int>(std::lround(diningTime * diningFactor)));
    student->setProfile(profile);

    if (!config_.windowCategories.empty()) {
        student->setPreferredCategory(utils::RandomGenerator::getUniformInt(
            0, static_cast<int>(config_.windowCategories.size()) - 1));
    }

    auto& window = chooseWindowWithPreference(student);
    student->setServiceTime(boundedServiceTimeForWindow(serviceTime, window));
    window.enqueue(student, currentTime_);
    return student;
}

void SimulationEngine::assignDiningGroups(int arrivalCount) {
    if (!config_.enableGroupDining || arrivalCount <= 0) return;
    const auto begin = allStudents_.end() - arrivalCount;
    for (auto it = begin; it != allStudents_.end(); ++it) {
        auto& student = *it;
        if (student->hasGroup() || student->isTakeaway()) continue;
        if (utils::RandomGenerator::getUniformDouble(0.0, 1.0) >= config_.groupRatio) continue;

        const int groupSize = utils::RandomGenerator::getUniformInt(config_.groupSizeMin, config_.groupSizeMax);
        const int groupId = nextStudentId_++;
        student->setGroupId(groupId);
        int placed = 1;
        for (auto jt = it + 1; jt != allStudents_.end() && placed < groupSize; ++jt) {
            auto& other = *jt;
            if (other->hasGroup() || other->isTakeaway()) continue;
            if (utils::RandomGenerator::getUniformDouble(0.0, 1.0) < 0.5) {
                other->setGroupId(groupId);
                ++placed;
            }
        }
    }
}

void SimulationEngine::generateArrivals() {
    if (currentTime_ >= config_.totalSimulationTime) return;

    const int arrivals = utils::RandomGenerator::getPoisson(config_.arrivalRatePerSecond(currentTime_));
    for (int i = 0; i < arrivals; ++i) {
        auto serviceTime = utils::RandomGenerator::getNormalInt(config_.avgServiceTime, config_.serviceStddev, 1);
        auto diningTime = utils::RandomGenerator::getNormalInt(config_.avgDiningTime, config_.diningStddev, 1);

        std::shared_ptr<Student> student;
        if (config_.enablePreferences) {
            student = createStudentWithPreferences(serviceTime, diningTime);
        } else {
            student = std::make_shared<Student>(nextStudentId_++, currentTime_, serviceTime, diningTime);
            auto& window = chooseWindow();
            student->setServiceTime(boundedServiceTimeForWindow(serviceTime, window));
            window.enqueue(student, currentTime_);
        }

        if (config_.enablePatience) {
            student->setPatience(utils::RandomGenerator::getNormalInt(
                static_cast<int>(config_.patienceBase), config_.patienceStddev, 10));
        }
        if (config_.enablePacking) {
            student->setTakeaway(utils::RandomGenerator::getUniformDouble(0.0, 1.0) < config_.packingRatio);
        }
        allStudents_.push_back(student);
    }
    assignDiningGroups(arrivals);
}

void SimulationEngine::tick() {
    generateArrivals();

    // 排队耐心：检查不耐烦的学生（窗口中排队等待超过耐心的）
    if (config_.enablePatience) {
        for (auto& window : windows_) {
            auto impatient = window.collectImpatient();
            for (auto& student : impatient) {
                if (config_.patienceAllowSwitch) {
                    // 找个其他窗口换队（优先队列最短的）
                    Window* bestOther = nullptr;
                    std::size_t bestLen = SIZE_MAX;
                    for (auto& w : windows_) {
                        if (&w == &window) continue;
                        const auto len = w.getQueueLength();
                        if (len < bestLen) {
                            bestLen = len;
                            bestOther = &w;
                        }
                    }
                    if (bestOther) {
                        bestOther->reEnqueue(student);
                        continue;  // 成功换队，不计数为放弃
                    }
                    // 没有其他窗口可去 -> 放弃
                }
                // 耐心耗尽，放弃离开
                ++droppedStudents_;
            }
        }
    }

    for (auto& window : windows_) {
        auto completed = window.tick(currentTime_);
        if (completed) {
            completed->setServedByWindowId(window.getId());
            if (config_.enablePacking && completed->isTakeaway()) {
                completed->startDining(currentTime_ + 1, -1, -1);
                completed->finishDiningAndLeave(currentTime_ + 1);
                finishedStudents_.push_back(completed);
            } else {
                waitingForSeat_.push_back(completed);
            }
        }
    }

    auto newlyFinished = canteen_.tick(currentTime_);
    for (auto& student : newlyFinished) {
        finishedStudents_.push_back(student);
    }

    // 工具 lambda：根据窗口 id 计算虚拟列
    auto windowVirtualCol = [&](int wid) -> int {
        if (wid < 1 || config_.windowCount <= 1) return config_.tableCols / 2;
        return (wid - 1) * (config_.tableCols - 1) / (config_.windowCount - 1);
    };

    // 等座分配：先处理结伴组，再处理个人
    while (!waitingForSeat_.empty() && canteen_.hasEmptySeat()) {
        auto front = waitingForSeat_.front();

        if (config_.enableGroupDining && front->hasGroup()) {
            const int gid = front->getGroupId();
            std::vector<std::shared_ptr<Student>> group;
            for (auto it = waitingForSeat_.begin(); it != waitingForSeat_.end(); ) {
                if ((*it)->getGroupId() == gid) {
                    group.push_back(*it);
                    it = waitingForSeat_.erase(it);
                } else {
                    ++it;
                }
            }
            const int wvc = windowVirtualCol(group[0]->getServedByWindowId());
            const int seated = canteen_.tryGroupSeat(group, currentTime_ + 1, wvc);
            for (int i = seated; i < static_cast<int>(group.size()); ++i) {
                waitingForSeat_.push_front(group[static_cast<std::size_t>(i)]);
            }
            if (seated == 0) break;
        } else {
            const int wvc = windowVirtualCol(front->getServedByWindowId());
            if (!canteen_.trySeat(front, currentTime_ + 1, wvc)) {
                break;
            }
            waitingForSeat_.pop_front();
        }
    }

    statistics_.record(currentTime_ + 1,
                       getTotalQueueLength(),
                       getWaitingForSeatCount(),
                       getOccupiedSeats(),
                       getFinishedStudentCount(),
                       canteen_.utilization());

    ++currentTime_;

    if (!finalized_ && isFinished()) {
        statistics_.finalize(finishedStudents_);
        finalized_ = true;
    }
}

void SimulationEngine::run() {
    const int safetyLimit = config_.totalSimulationTime + std::max(7200, config_.avgDiningTime * 10 + config_.avgServiceTime * config_.windowCount * 50);
    while (!isFinished()) {
        if (currentTime_ > safetyLimit) {
            throw std::runtime_error("simulation safety limit reached; check configuration capacity and arrival rate");
        }
        tick();
    }
    statistics_.finalize(finishedStudents_);
}

bool SimulationEngine::hasActiveStudents() const noexcept {
    for (const auto& window : windows_) {
        if (!window.empty()) {
            return true;
        }
    }
    return !waitingForSeat_.empty() || canteen_.getOccupiedSeats() > 0;
}

bool SimulationEngine::isFinished() const noexcept {
    return currentTime_ >= config_.totalSimulationTime && !hasActiveStudents();
}

int SimulationEngine::getTotalQueueLength() const noexcept {
    int total = 0;
    for (const auto& window : windows_) {
        total += static_cast<int>(window.getQueueLength());
    }
    return total;
}

} // namespace bdss::core
