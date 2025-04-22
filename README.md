# AGRender

**AGRender** is a high-performance rendering engine for visualizing results from N-body simulations, specifically designed to produce photorealistic imagery of galactic structures and astrophysical phenomena. It leverages modern **OpenGL** techniques and custom **GLSL shaders** to render millions of particles in real-time, using scientifically meaningful color maps and physically-inspired lighting models.

This tool enables researchers and enthusiasts to explore and present cosmic-scale simulations with both scientific accuracy and cinematic visual fidelity.

---

## 🔧 Technical Overview

AGRender is built on a C++ backend using **OpenGL 4.5+**, taking full advantage of GPU acceleration for rendering dense particle datasets. Key rendering techniques include:

- **Deferred shading pipeline** for high-performance lighting effects
- **Physically-based particle luminance calculation**, driven by stellar and gas components
- **Custom GLSL fragment and vertex shaders** to control point sprite rendering, blending, and coloring
- **Efficient camera control system** for both real-time navigation and scripted flythroughs
- Support for **high dynamic range (HDR)** rendering and tone mapping
- Optimized data transfer via **Vertex Buffer Objects (VBOs)** and **Uniform Buffer Objects (UBOs)**

---

## 📁 Supported Formats

AGRender supports a wide range of astrophysical simulation formats, ensuring seamless integration with major tools and workflows:

- **Astrogen2 formats**:
  - `.ag`
  - `.agc`
  - `.age`
- **Gadget legacy formats** (including GADGET-1 and GADGET-2 binary snapshots)
  - Single-file and multi-file snapshot support
  - Automatic endian conversion
  - Unit conversion for consistent rendering

---

## 🎨 Visualization Features

- Real-time 3D rendering of particle-based astrophysical simulations
- Support for multiple scientific quantities:
  - Mass
  - Velocity
  - Density
  - Temperature
  - Luminosity
- Advanced color maps:
  - `Viridis`
  - `Inferno`
  - `Jet`
  - Custom **photorealistic galactic colormap** with background stars and nebular overlays
- Alpha blending for gas and stellar components to simulate volume and glow
- Adjustable color scaling, thresholds, and transparency

---

## 🎥 Key Features

- **Interactive 3D View**:
  - Free camera navigation
  - Centered mode to lock view on specific objects or regions
- **Video Rendering Mode**:
  - Predefined camera tracks through 3D space
  - Frame-by-frame output for high-quality animations and presentations

---

## 🚀 Example Use Cases

- Visualizing galaxy mergers with gas and star separation
- Rendering star formation regions using temperature-based colormaps
- Cinematic flythroughs of simulated dark matter halos
- Generating static renders for publications, presentations, or outreach

**Example renders:**

Milky Way Andromeda merge rendered with our photorealistic galactic colormap:

![Unbenannt](https://github.com/user-attachments/assets/3b3122b5-c628-4eeb-bcbe-d90348ee1afd)

Dark Matter halo merge rendered with the viridris colormap:

![Unbenannt2](https://github.com/user-attachments/assets/b4732f13-21e0-43b7-a6d1-f6f3f2aebaff)

Stars rendered with the inferno colormap:

![Unbenannt3](https://github.com/user-attachments/assets/241cda8e-547b-4ec5-9bcf-4d9ce673d345)

Interstellar gas rendered with the jet colormap:

---

## 📄 License

AGRender is released under the **GNU General Public License v3.0 (GPL-3.0)**.  
See the [LICENSE](./LICENSE) file for more details.

---

## 📦 Installation & Usage

See Getting_Started.md
