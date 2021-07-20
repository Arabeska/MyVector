#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <iostream>
#include <algorithm>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    RawMemory(RawMemory&& other) noexcept {
        Swap(other);
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        buffer_ = std::move(rhs.buffer_);
        rhs.buffer_ = nullptr;
        capacity_ = rhs.capacity_;
        rhs.capacity_ = 0;
        return *this;
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

private:
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};


template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size) {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_) {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_))
        , size_(other.size_) {
        other.size_ = 0;
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else {
                std::copy_n(rhs.data_.GetAddress(), ((size_ < rhs.size_) ? size_ : rhs.size_), data_.GetAddress());
                if (size_ < rhs.size_) {
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.Size() - size_, data_.GetAddress() + size_);
                }
                else if (rhs.size_ < size_){
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                }
                size_ = rhs.Size();
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) noexcept {
        Swap(rhs);
        return *this;
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator begin() const noexcept {
        return cbegin();
    }

    const_iterator end() const noexcept {
        return cend();
    }

    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
        return data_.GetAddress() + size_;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        MakeInitialized(data_.GetAddress(), new_data.GetAddress(), size_);
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
        }
        else {
            if (new_size > data_.Capacity()) {
                Reserve(new_size);
            }
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        }
        size_ = new_size;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ + 1 > data_.Capacity()) {
            Vector<T> nv(0);
            nv.Reserve(size_ == 0 ? 1 : size_ * 2);
            new (nv.data_.GetAddress() + size_) T{ std::forward<Args>(args)... };
            MakeInitialized(data_.GetAddress(), nv.data_.GetAddress(), size_);
            nv.size_ = size_ + 1;
            Swap(nv);
        }
        else {
            new (data_.GetAddress() + size_) T{ std::forward<Args>(args)... };
            size_++;
        }
        return data_[size_ - 1];
    }

    template <typename Val>
    void PushBack(Val&& value) {
        EmplaceBack(std::forward<Val>(value));
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        if (pos == cend()) {
            EmplaceBack(std::forward<Args>(args)...);
            return end() - 1;
        }
        size_t offset = pos - begin();
        size_t number = cend() - pos;
        if (size_ + 1 > data_.Capacity()) {
            Vector<T> nv(0);
            nv.Reserve(size_ == 0 ? 1 : size_ * 2);
            new (nv.data_.GetAddress() + offset) T{ std::forward<Args>(args)... };
            if (pos != begin()) {
                MakeInitialized(data_.GetAddress(), nv.data_.GetAddress(), offset);
            }
            MakeInitialized(data_.GetAddress() + offset, nv.data_.GetAddress() + offset + 1, number);
            nv.size_ = size_ + 1;
            Swap(nv);
        }
        else {
            T t(std::forward<Args>(args)...);
            std::uninitialized_move_n(data_.GetAddress() + size_ - 1, 1, data_.GetAddress() + size_);
            std::move_backward(data_.GetAddress() + offset, end() - 1, end());
            *(data_.GetAddress() + offset) = std::move(t);
            size_++;
        }

        return begin() + offset;
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

    void PopBack() noexcept {
        assert(size_ > 0);
        std::destroy_n(data_.GetAddress() + size_ - 1, 1);
        size_--;
    }

    iterator Erase(const_iterator pos) {
        if (pos == end() - 1) {
            PopBack();
            return end() - 1;
        }
        size_t number = pos - begin();
        std::move(begin() + number + 1, end(), begin() + number);
        Resize(size_ - 1);
        return begin() + number;
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    template <typename It>
    inline void MakeInitialized(It from, It where, size_t number) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(from, number, where);
        }
        else {
            std::uninitialized_copy_n(from, number, where);
        }
    }
};


