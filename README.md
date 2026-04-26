# AutomataDungeone

**AutomataDungeone** is a high-performance, statically-linked C++23 surface generator designed for creating complex, layered terrain maps. It uses an architecture of layer which provides composition API through variadic template `Pipeline` that allows for type-safe, compile-time composition of generation layers with lazy evaluation.

## Architecture: The Layered Approach

The project is built on three core pillars that enable highly flexible map generation:

### 1. The Grid (Data)

The `Grid` is the central data structure representing the map. Each `Cell` contains:

- `height`: Normalized elevation (0.0 to 1.0).
- `forest_mask`: Probability or density mask for vegetation.
- `textureId`: Visual representation (0: Grass, 1: Mountain, 2: Tree).

### 2. Generator Layers (Logic)

A **Layer** is any struct that satisfies the `GeneratorLayer` concept. This requires two methods:

- `apply(Grid& grid)`: Performs the transformation.
- `get_name()`: Returns a string identifier for debugging/logging.

### 3. Static Pipeline (Composition)

The `Pipeline` is a variadic template that composes multiple layers. It uses C++17 fold expressions and `std::apply` to iterate over layers at compile-time, ensuring zero-cost abstraction and short-circuiting error handling.

## Generation Formulas

Each layer uses specific mathematical models to transform the grid:

### Landscape Generation

Uses a radial sine-wave distance field to create organic, island-like or cave-like formations.
$$\text{height} = \text{clamp}(0.5 + 0.5 \cdot \sin(\text{distance} + \text{frequency}), 0, 1)$$
where `distance` is the normalized Euclidean distance from the center of the grid.

### Forest Masking

Calculates where trees _could_ grow based on elevation. It uses an inverse linear falloff from a specified height limit.
$$\text{forest\_mask} = \begin{cases} 0.0 & \text{if } \text{height} > \text{limit} \\ 1.0 - \frac{\text{height}}{\text{limit}} & \text{otherwise} \end{cases}$$

### Forest Placement

A stochastic layer that transforms the mask into actual tree placements using a noise-based probability check.
$$\text{is\_tree} = \text{random}(0, 1) < (\text{forest\_mask} \cdot \text{density})$$

## Usage

### Building

The project requires a modern Clang toolchain (LLVM 18+) and links statically against `libc++`.

```bash
make clean && make
```

### Running

Executing the binary will run the default pipeline and output an ASCII representation of the generated dungeone.

```bash
./AutomataDungeone
```

## Example Composition

```cpp
auto pipeline = Pipeline(
    LandscapeGenerator{12.0f},   // Create terrain
    ForestMaskGenerator{0.6f},   // Find fertile areas
    ForestPlacementGenerator{0.5f} // Plant trees
);
pipeline.execute(*grid);
```

## Example CLI mode output

```
^^^^^^^^^^^...........................................^^^^^^^^^^
^^^^^^^^.................................................^^^^^^^
^^^^^^.....................................................^^^^^
^^^^.........................................................^^^
^^.............................................................^
^...............................................................
..........................Y.........Y...........................
........................Y........Y..............................
................................................................
............Y...................................................
.........................Y...YY....Y....Y.......................
.....................Y......Y.Y..Y.Y............................
.................Y..........................YY..Y...............
................................Y....YY....Y..Y.................
..............................Y....YY...Y...Y...................
...................Y....Y.......YY..Y..Y........................
...................Y.Y...YYY......Y...Y.........................
..........................Y.Y....Y.Y..........Y.................
....................Y.......Y..YY.....Y.....Y.Y.................
.....................Y..Y...Y...Y.......Y.Y.YY..................
................Y............YY.Y..Y.........Y..................
.............................YY...Y.............................
..................Y......Y.Y.Y.....Y.Y.......Y..Y...............
....................Y...............Y..Y...Y....................
.................................Y....YY........................
...................................Y............................
..............................Y.................................
^...............................................................
^^...................................Y.........................^
^^^^.........................................................^^^
^^^^^^.....................................................^^^^^
^^^^^^^^.................................................^^^^^^^
```
