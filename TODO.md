
The big OpenDrakan TODO list
============================

This list contains all the features and details of OpenDrakan that still need to be implemented or require more work.
Feel free to pick an item and open a ticket for it should you wish to work on a feature.

- Multiplayer
    - Biggest hurdle to progress yet. Nearly all game logic should interact with this, so until this stands, progress with game logic is stalled
    - Protocol compatibility has low priority. If the original protocol is the source of bugs, or simply can't be reverse-engineered sufficiently, scratch it
    - Since handling multiplayer probably involves some incremental state tracking, this can probably be leveraged for easily implementing savegames
- Drakan GUI
    - GUI framework needs refinement
        - Support for layout building blocks (boxes, scrollable widgets etc.)
        - Animations
    - Text rendering support
    - Widgets required:
        - ~~Health orb~~
        - ~~Main menu~~
        - Dragon power orb
        - Breath meter
        - Loading bar
        - Settings menu (pretty complex)
    - Main menu crystal buttons still look wrong (crystal motion and lighting)
- Savegames
    - Backwards compatibility is almost completely irrelevant here
    - Might be easier to leverage multiplayer code for this
- Skeletal animation code needs work
    - Needs support for inverse kinematics
    - Movements of skeleton need to apply to bounding data
    - ~~Needs support for multiple interpolation styles (mostly "no interpolation" for authentic Drakan animations)~~
- STOMP sequences
- Renderer synchronization
    - Access synchronization to the rendering subsystem is inconsistently used or even absent in many cases. This needs a
      new efficient and effective concept.
    - The render loop and the update loop are not synchronized. Thus, it is possible for the renderer to render a partially updated scene,
      resulting in things like the skydome stuttering as a frame occurs between updating the sky and camera
        - A possible elegant solution would be to define "update chains" in the renderer, preventing it from rendering objects with update dependencies
          out of order
- Physics performance
    - The physics system is the biggest hog of game logic loop time right now. There might be some optimizations possible, like
      a broadphase that handles layers separately.
    - Since lights are exclusively spherical in nature, they can be even more optimized with an M-Tree broadphase
- RFL classes
    - Most of the class types still need to be implemented
        - Keep in mind that these will likely require close interaction with the multiplayer code
    - Many classes' original behaviour still needs to be researched (will add list of implemented/documented classes soon)
- Layer PVS
    - We shouldn't spawn objects and layers until they are deemed visible by the current layer
    - ~~A stronger association between objects and the layers they are "on" might be necessary~~
        - ~~Should consider objects that hang off the ceiling, too~~
- DLS loading in music subsystem
    - The FluidSynth based subsystem does not like Drakan's DLS files, also requires big dependency for DLS support
    - Might want to try out a different synth altogether
- Character controllers
    - Character controlled by player
    - Dragon controlled by player
    - Character controlled by AI
    - Dragon controlled by AI
- AI
- SFX handling
    - If something needs to play SFX, right now that means it has to create it's own sound source. While this obviously works, it might
      be sub-optimal for performance. It'd be better to have a priority queue that distributes the SFX across a few sources.
- Pure OpenGL (or even Vulkan) renderer
    - OSG is a pretty heavy dependency and we only use a small subset of it. A better, pure OpenGL renderer shouldn't be too hard to implement
        