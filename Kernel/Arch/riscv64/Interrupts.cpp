/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Error.h>
#include <AK/Types.h>

#include <Kernel/Arch/CPU.h>
#include <Kernel/Arch/Interrupts.h>
#include <Kernel/Arch/TrapFrame.h>
#include <Kernel/Interrupts/GenericInterruptHandler.h>
#include <Kernel/Interrupts/SharedIRQHandler.h>
#include <Kernel/Interrupts/UnhandledInterruptHandler.h>

namespace Kernel {

// FIXME: Share this array with x86_64/aarch64 somehow and consider if this really needs to use raw pointers and not OwnPtrs
static Array<GenericInterruptHandler*, 64> s_interrupt_handlers;

void dump_registers(RegisterState const&)
{
}

// FIXME: Share the code below with Arch/x86_64/Interrupts.cpp
//        While refactoring, the interrupt handlers can also be moved into the InterruptManagement class.
GenericInterruptHandler& get_interrupt_handler(u8)
{
    TODO_RISCV64();
}

// Sets the reserved flag on `number_of_irqs` if it finds unused interrupt handler on
// a contiguous range.
ErrorOr<u8> reserve_interrupt_handlers(u8)
{
    TODO_RISCV64();
}

static void revert_to_unused_handler(u8 interrupt_number)
{
    auto* handler = new UnhandledInterruptHandler(interrupt_number);
    handler->register_interrupt_handler();
}

// FIXME: Share the code below with Arch/{x86_64,aarch64}/Interrupts.cpp
void register_generic_interrupt_handler(u8 interrupt_number, GenericInterruptHandler& handler)
{
    auto*& handler_slot = s_interrupt_handlers[interrupt_number];
    if (handler_slot == nullptr) {
        handler_slot = &handler;
        return;
    }
    if (handler_slot->type() == HandlerType::UnhandledInterruptHandler) {
        auto* unhandled_handler = static_cast<UnhandledInterruptHandler*>(handler_slot);
        unhandled_handler->unregister_interrupt_handler();
        delete unhandled_handler;
        handler_slot = &handler;
        return;
    }
    if (handler_slot->is_shared_handler()) {
        VERIFY(handler_slot->type() == HandlerType::SharedIRQHandler);
        static_cast<SharedIRQHandler*>(handler_slot)->register_handler(handler);
        return;
    }
    if (!handler_slot->is_shared_handler()) {
        if (handler_slot->type() == HandlerType::SpuriousInterruptHandler) {
            // FIXME: Add support for spurious interrupts on riscv64
            TODO_RISCV64();
        }
        VERIFY(handler_slot->type() == HandlerType::IRQHandler);
        auto& previous_handler = *handler_slot;
        handler_slot = nullptr;
        SharedIRQHandler::initialize(interrupt_number);
        VERIFY(handler_slot);
        static_cast<SharedIRQHandler*>(handler_slot)->register_handler(previous_handler);
        static_cast<SharedIRQHandler*>(handler_slot)->register_handler(handler);
        return;
    }
    VERIFY_NOT_REACHED();
}

// FIXME: Share the code below with Arch/{x86_64,aarch64}/Interrupts.cpp
void unregister_generic_interrupt_handler(u8 interrupt_number, GenericInterruptHandler& handler)
{
    auto*& handler_slot = s_interrupt_handlers[interrupt_number];
    VERIFY(handler_slot != nullptr);
    if (handler_slot->type() == HandlerType::UnhandledInterruptHandler)
        return;
    if (handler_slot->is_shared_handler()) {
        VERIFY(handler_slot->type() == HandlerType::SharedIRQHandler);
        auto* shared_handler = static_cast<SharedIRQHandler*>(handler_slot);
        shared_handler->unregister_handler(handler);
        if (shared_handler->sharing_devices_count() == 0) {
            handler_slot = nullptr;
            revert_to_unused_handler(interrupt_number);
        }
        return;
    }
    if (!handler_slot->is_shared_handler()) {
        VERIFY(handler_slot->type() == HandlerType::IRQHandler);
        handler_slot = nullptr;
        revert_to_unused_handler(interrupt_number);
        return;
    }
}

void initialize_interrupts()
{
    for (size_t i = 0; i < s_interrupt_handlers.size(); ++i) {
        auto* handler = new UnhandledInterruptHandler(i);
        handler->register_interrupt_handler();
    }
}

}
