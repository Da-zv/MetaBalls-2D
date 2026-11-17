# MetaBalls-2D
A little experiment with balls, math, and blobs.

## About

MetaBalls-2D is a small graphics programming project focused on rendering and simulating 2D metaballs in real time.

Metaballs are organic shapes that smoothly merge together when they get close to each other, creating a fluid-like effect.

The project uses an influence-based field calculation to determine which parts of the screen should be rendered.

## Features

Real-time 2D metaball rendering

Dynamic metaball movement

Smooth merging and separation of metaballs

Adjustable metaball radius

Configurable influence threshold

Reduced-resolution rendering for performance

Multithreaded CPU calculations

## Preview

<img width="802" height="632" alt="image" src="https://github.com/user-attachments/assets/4b314d42-6b1d-47bc-af4f-14fd294ad9c2" />

## How It Works

Each metaball contributes to a scalar influence field based on its distance from a point.

The influence is calculated using:

influence = radius * radius / distanceSquared;

The influences of all metaballs are added together.

If the total influence exceeds a specified threshold, that point is rendered.

This creates the characteristic smooth merging effect.

## Built With

FreeGLUT - window creation, input handling and OpenGL context.
GLM - vector and mathematical operations.
OpenGL - rendering.

## Performance

The project explores different techniques for improving rendering performance:

Reduced-resolution rendering

Early termination of influence calculations

CPU multithreading

Parallel computation

## Future Ideas

GPU-based metaball calculations using GLSL

Adjustable metaball colors

Performance benchmarking

Thread pool implementation

Smooth color gradients

Configurable simulation settings
