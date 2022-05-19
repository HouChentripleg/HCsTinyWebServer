#include "../include/timer.hpp"

/*  此处使用vector构造的小顶堆(完全二叉树)
    父节点索引为x，左节点为2x+1，右节点为2x+2
*/
void TimerManager::swapNode(size_t i, size_t j) {
    assert(i >= 0 && i < heapTimer_.size());
    assert(j >= 0 && j < heapTimer_.size());
    // 交换堆中的两个定时器节点
    std::swap(heapTimer_[i], heapTimer_[j]);
    // 更新哈希表中定时器id在堆中的索引下标
    ref_[heapTimer_[i].id] = i;
    ref_[heapTimer_[j].id] = j;
}

void TimerManager::siftup(size_t i) {
    assert(i >= 0 && i < heapTimer_.size());
    int j = (i - 1) / 2;  // 获取节点i的父节点
    while(j >= 0) {
        // 利用重载的operator<
        if(heapTimer_[j] < heapTimer_[i]) {
            break;
        }
        // 父节点>子节点时长，交换
        swapNode(i, j);
        i = j;  // 新索引
        j = (i - 1) / 2;  // 新父节点索引
    }
}

void TimerManager::siftdown(size_t i) {
    int n = heapTimer_.size();
    assert(i >= 0 && i < n);
    int j = 2 * i + 1;  // 获取左子节点
    while(j < n) {
        if(heapTimer_[i] < heapTimer_[j]) {
            break;
        }
        if((j + 1 < n) && heapTimer_[j + 1] < heapTimer_[j]) {
            // 取左、右子节点中的min
            ++j;
        }
        // 交换节点i和节点min
        swapNode(i, j);
        // 更新索引
        i = j;
        j = 2 * i + 1;
    }
}

void TimerManager::addTimer(int id, int timewait, const TimeoutCallBack& cbfunc) {
    assert(id >= 0);
    size_t i;
    if(ref_.count(id) == 0) {
        // 新节点，插在堆尾，siftup()调整堆
        i = heapTimer_.size();
        ref_[id] = i;
        heapTimer_.push_back({id, CLOCK::now() + MS(timewait), cbfunc});
        siftup(i);
    } else {
        // 已有节点，调整堆
        i = ref_[id];
        heapTimer_[i].timeExpire = CLOCK::now() + MS(timewait);
        heapTimer_[i].callbackFunc = cbfunc;
        siftup(i);
        siftdown(i);
    }
}

void TimerManager::delTimer(size_t i) {
    assert(!heapTimer_.empty() && i >= 0 && i < heapTimer_.size());
    int n = heapTimer_.size() - 1;
    swapNode(i, n);
    ref_.erase(heapTimer_.back().id);
    heapTimer_.pop_back();
    if(!heapTimer_.empty()) {
        // 堆非空，调整堆
        siftup(i);
        siftdown(i);
    }
}

void TimerManager::work(size_t id) {
    if(heapTimer_.empty() || ref_.count(id) == 0) {
        return;
    }
    int i = ref_[id];
    TimerNode node = heapTimer_[i];
    node.callbackFunc();
    delTimer(i);
}

void TimerManager::update(size_t id, int timewait) {
    assert(!heapTimer_.empty() && ref_.count(id) > 0);
    int i = ref_[id];
    heapTimer_[i].timeExpire = CLOCK::now() + MS(timewait);
    // 时间变长，向下调整
    siftdown(i);
}

void TimerManager::handleExpiredTimer() {
    if(heapTimer_.empty()) {
        return;
    }
    while(!heapTimer_.empty()) {
        TimerNode node = heapTimer_.front();
        if(std::chrono::duration_cast<MS>(node.timeExpire - CLOCK::now()).count() > 0) {
            // 定时未到
            break;
        }
        node.callbackFunc();    // 关闭对应的HTTP连接
        pop();  // 删除到期的定时器
    }
}

void TimerManager::pop() {
    assert(!heapTimer_.empty());
    delTimer(0);  // 删除最前端的定时器
}

void TimerManager::clear() {
    ref_.clear();
    heapTimer_.clear();
}

int TimerManager::getNextHandle() {
    handleExpiredTimer();
    int res = -1;
    if(!heapTimer_.empty()) {
        // 获取下一次处理的时长
        res = std::chrono::duration_cast<MS>(heapTimer_.front().timeExpire - CLOCK::now()).count();
        if(res < 0) {
            res = 0;
        }
    }
    return res;
}