fn main() {
    // Tell Cargo that if the memory.x file changes, to rebuild
    println!("cargo:rerun-if-changed=memory.x");

    // Add linker argument for memory layout
    println!("cargo:rustc-link-arg=-Tmemory.x");
    
    // Specify we want to link against the ARM Cortex-M4F with hardware FPU
    println!("cargo:rustc-link-arg=-mcpu=cortex-m4");
    println!("cargo:rustc-link-arg=-mthumb");
    println!("cargo:rustc-link-arg=-mfloat-abi=hard");
    println!("cargo:rustc-link-arg=-mfpu=fpv4-sp-d16");
}
