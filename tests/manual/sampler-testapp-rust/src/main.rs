// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

//! Manual test target for the qtprofiler sampling profiler, written in Rust.
//!
//! This is the language counterpart to the C++ `sampler-testapp`: it spawns the
//! same set of named threads with distinctive call stacks and CPU behaviors so
//! the sampler can be exercised against native Rust binaries too. Set
//! `RecordProcessName=sampler-testapp-rust` in the viewer settings, then record:
//!   - "fibonacci": always busy in a deep recursive chain (heaviest path)
//!   - "hasher":    always busy in a second hot root (xorshift mixing)
//!   - "bursty":    100 ms busy / 400 ms asleep (CPU-usage wave)
//!   - "sleeper":   almost always blocked (should nearly vanish from the tree)
//!   - main:        idle loop with a 1 s heartbeat (low-weight stack)
//!
//! Unlike the C++ app there is no Qt Quick / Qt Quick 3D window: a pure-Rust
//! build would need a heavy GUI dependency to drive render threads, so this
//! variant focuses on the CPU-bound worker threads that are the sampler's core
//! subject. Ctrl+C stops it.

use std::hint::black_box;
use std::process;
use std::sync::atomic::{AtomicBool, Ordering};
use std::thread::{self, JoinHandle};
use std::time::{Duration, Instant};

static STOP_REQUESTED: AtomicBool = AtomicBool::new(false);

// Every function in a chain is marked #[inline(never)] and pushes its work
// through black_box, so the optimizer cannot collapse the frames and the
// sampler sees each function as its own stack level.

#[inline(never)]
fn fib(n: u64) -> u64 {
    if n < 2 {
        return n;
    }
    fib(n - 1) + fib(n - 2)
}

#[inline(never)]
fn compute_fibonacci() {
    black_box(fib(black_box(30)));
}

#[inline(never)]
fn fibonacci_worker() {
    while !STOP_REQUESTED.load(Ordering::Relaxed) {
        compute_fibonacci();
    }
}

#[inline(never)]
fn mix_bytes(mut state: u64) -> u64 {
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    state.wrapping_mul(0x2545_F491_4F6C_DD1D) // xorshift* multiplier
}

#[inline(never)]
fn hash_block(seed: u64) -> u64 {
    let mut state = seed | 1;
    for _ in 0..100_000 {
        state = mix_bytes(state);
    }
    state
}

#[inline(never)]
fn hasher_worker() {
    let mut sink: u64 = 0;
    while !STOP_REQUESTED.load(Ordering::Relaxed) {
        sink = hash_block(black_box(sink));
    }
    black_box(sink);
}

#[inline(never)]
fn spin_for(ms: u64) {
    let timer = Instant::now();
    let budget = Duration::from_millis(ms);
    let mut sink: u64 = 0;
    while timer.elapsed() < budget && !STOP_REQUESTED.load(Ordering::Relaxed) {
        sink = mix_bytes(black_box(sink) + 1);
    }
    black_box(sink);
}

#[inline(never)]
fn burst_of_work() {
    spin_for(100);
    thread::sleep(Duration::from_millis(400));
}

#[inline(never)]
fn bursty_worker() {
    while !STOP_REQUESTED.load(Ordering::Relaxed) {
        burst_of_work();
    }
}

#[inline(never)]
fn wait_around() {
    thread::sleep(Duration::from_millis(1000));
}

#[inline(never)]
fn sleeper_worker() {
    while !STOP_REQUESTED.load(Ordering::Relaxed) {
        wait_around();
    }
}

fn start_worker(name: &str, worker: fn()) -> JoinHandle<()> {
    // The thread name becomes the OS thread name the sampler reads: Rust calls
    // pthread_setname_np from inside the spawned thread on macOS and Linux.
    thread::Builder::new()
        .name(name.to_owned())
        .spawn(worker)
        .expect("failed to spawn worker thread")
}

fn main() {
    println!("sampler-testapp-rust running, pid {}", process::id());
    println!(
        "Set RecordProcessName=sampler-testapp-rust in the trace viewer settings, \
         then record. Ctrl+C stops."
    );

    let workers = [
        start_worker("fibonacci", fibonacci_worker),
        start_worker("hasher", hasher_worker),
        start_worker("bursty", bursty_worker),
        start_worker("sleeper", sleeper_worker),
    ];

    // Idle main thread with a 1 s heartbeat, mirroring the C++ event-loop timer.
    let mut beat = 0;
    while !STOP_REQUESTED.load(Ordering::Relaxed) {
        thread::sleep(Duration::from_millis(1000));
        beat += 1;
        println!("heartbeat {beat}");
    }

    for worker in workers {
        let _ = worker.join(); // sleeper may take up to 1 s to notice the flag
    }
    println!("stopped");
}
