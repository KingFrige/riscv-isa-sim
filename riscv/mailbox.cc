#include "mailbox.h"
#include "processor.h"
#include "sim.h"
#include <cstring>
#include <iostream>

mailbox_t::mailbox_t(const simif_t* sim)
    : sim(sim),
      status_reg(MAILBOX_READY),  // 初始状态为就绪
      command_reg(0),
      data_addr_reg(0),
      data_size_reg(0),
      response_reg(MAILBOX_SUCCESS),
      vector_config_reg(0),
      command_handler(nullptr)
{
    // 初始状态：就绪，不忙，无错误，非向量模式
    update_status(true, false, false, false);
}

bool mailbox_t::load(reg_t addr, size_t len, uint8_t* bytes)
{
    if (!validate_address(addr, len)) {
        return false;
    }
    
    // 根据偏移地址读取相应的寄存器
    reg_t offset = addr - MAILBOX_BASE;
    
    switch (offset) {
        case MAILBOX_STATUS_OFFSET:
            if (len == 4) {
                memcpy(bytes, &status_reg, 4);
                return true;
            }
            break;
            
        case MAILBOX_COMMAND_OFFSET:
            if (len == 4) {
                memcpy(bytes, &command_reg, 4);
                return true;
            }
            break;
            
        case MAILBOX_DATA_ADDR_OFFSET:
            if (len == 8) {
                memcpy(bytes, &data_addr_reg, 8);
                return true;
            } else if (len == 4) {
                // 读取低32位
                uint32_t low = static_cast<uint32_t>(data_addr_reg);
                memcpy(bytes, &low, 4);
                return true;
            }
            break;
            
        case MAILBOX_DATA_SIZE_OFFSET:
            if (len == 4) {
                memcpy(bytes, &data_size_reg, 4);
                return true;
            }
            break;
            
        case MAILBOX_RESPONSE_OFFSET:
            if (len == 4) {
                memcpy(bytes, &response_reg, 4);
                return true;
            }
            break;
            
        case MAILBOX_VECTOR_CONFIG_OFFSET:
            if (len == 8) {
                memcpy(bytes, &vector_config_reg, 8);
                return true;
            } else if (len == 4) {
                // 读取低32位
                uint32_t low = static_cast<uint32_t>(vector_config_reg);
                memcpy(bytes, &low, 4);
                return true;
            }
            break;
            
        default:
            // 未映射的地址返回0
            memset(bytes, 0, len);
            return true;
    }
    
    return false;
}

bool mailbox_t::store(reg_t addr, size_t len, const uint8_t* bytes)
{
    if (!validate_address(addr, len)) {
        return false;
    }
    
    reg_t offset = addr - MAILBOX_BASE;
    
    switch (offset) {
        case MAILBOX_STATUS_OFFSET:
            if (len == 4) {
                uint32_t new_status;
                memcpy(&new_status, bytes, 4);
                
                // 写入状态寄存器会触发命令执行
                // 任何写入都会清除错误位并触发处理
                if (new_status != 0) {
                    // 清除错误位
                    status_reg &= ~MAILBOX_ERROR;
                    
                    // 如果当前就绪且不忙，开始处理命令
                    if ((status_reg & MAILBOX_READY) && !(status_reg & MAILBOX_BUSY)) {
                        process_command();
                    }
                }
                return true;
            }
            break;
            
        case MAILBOX_COMMAND_OFFSET:
            if (len == 4) {
                memcpy(&command_reg, bytes, 4);
                return true;
            }
            break;
            
        case MAILBOX_DATA_ADDR_OFFSET:
            if (len == 8) {
                memcpy(&data_addr_reg, bytes, 8);
                return true;
            } else if (len == 4) {
                // 写入低32位，高32位保持不变
                uint32_t low;
                memcpy(&low, bytes, 4);
                data_addr_reg = (data_addr_reg & 0xFFFFFFFF00000000ULL) | low;
                return true;
            }
            break;
            
        case MAILBOX_DATA_SIZE_OFFSET:
            if (len == 4) {
                memcpy(&data_size_reg, bytes, 4);
                return true;
            }
            break;
            
        case MAILBOX_RESPONSE_OFFSET:
            if (len == 4) {
                // 响应寄存器通常只读，但允许写入以清除错误状态
                memcpy(&response_reg, bytes, 4);
                return true;
            }
            break;
            
        case MAILBOX_VECTOR_CONFIG_OFFSET:
            if (len == 8) {
                memcpy(&vector_config_reg, bytes, 8);
                return true;
            } else if (len == 4) {
                // 写入低32位，高32位保持不变
                uint32_t low;
                memcpy(&low, bytes, 4);
                vector_config_reg = (vector_config_reg & 0xFFFFFFFF00000000ULL) | low;
                return true;
            }
            break;
            
        default:
            // 忽略对未映射地址的写入
            return true;
    }
    
    return false;
}

void mailbox_t::process_command()
{
    // 设置忙状态
    update_status(false, true, false, vector_config_reg != 0);
    
    // 如果有命令处理器，调用它
    if (command_handler) {
        response_reg = command_handler(command_reg, data_addr_reg, data_size_reg, vector_config_reg);
        
        if (response_reg != MAILBOX_SUCCESS) {
            // 命令执行失败，设置错误位
            update_status(true, false, true, vector_config_reg != 0);
        } else {
            // 命令执行成功
            update_status(true, false, false, vector_config_reg != 0);
        }
    } else {
        // 没有命令处理器，返回未实现错误
        response_reg = MAILBOX_ERR_NOT_IMPLEMENTED;
        update_status(true, false, true, vector_config_reg != 0);
    }
}

void mailbox_t::update_status(bool ready, bool busy, bool error, bool vector_mode)
{
    status_reg = 0;
    if (ready) status_reg |= MAILBOX_READY;
    if (busy) status_reg |= MAILBOX_BUSY;
    if (error) status_reg |= MAILBOX_ERROR;
    if (vector_mode) status_reg |= MAILBOX_VECTOR_MODE;
}

bool mailbox_t::validate_address(reg_t addr, size_t len) const
{
    return (addr >= MAILBOX_BASE) && (addr + len <= MAILBOX_BASE + MAILBOX_SIZE);
}

// 模板函数定义
template<typename T>
T mailbox_t::read_register(reg_t offset) const
{
    // 这是一个简单的实现，实际中可能需要根据偏移地址读取不同的寄存器
    // 这里返回0作为占位符
    return T(0);
}

template<typename T>
void mailbox_t::write_register(reg_t offset, T value)
{
    // 这是一个简单的实现，实际中可能需要根据偏移地址写入不同的寄存器
    // 这里不做任何操作作为占位符
    (void)offset;
    (void)value;
}

// 模板实例化
template uint32_t mailbox_t::read_register<uint32_t>(reg_t offset) const;
template uint64_t mailbox_t::read_register<uint64_t>(reg_t offset) const;

template void mailbox_t::write_register<uint32_t>(reg_t offset, uint32_t value);
template void mailbox_t::write_register<uint64_t>(reg_t offset, uint64_t value);