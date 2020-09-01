# Functionality Breakdown
- **Player**
	- You are always spawned in the same world x/z with the y adjusted based on terrain. Biome might be different. ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/spawn.png))
	- There is no rendered player character. The camera view serves to move the character position.
	- Movement with WASD keys, not rebindable using the keybinding window in its existing state (would need a functionality extension for the keybinding system to work properly for it).
	- Jump with space by default; this is rebindable.
- **Enemies**
	- A* pathfinding is used on all mob movement. This is "layered", which considerably optimizes it from a raw pathfind which can be quite slow.
	- Enemies in dungeons come in waves, increasing until the dungeon wave count specified, after which the dungeon is completed. ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/waves.png))
	- Simple stat calculations are in effect, and the stats given to an enemy apply to how it interacts with the world and in combat.
	- Enemy stats scale each wave and slightly increase with no inherent limit (the number of waves determines the cap).
	- There is a dungeon difficulty multiplier that scales the mobs by a factor depending on the desired difficulty set.
	- There is a little bit of mob diversity for each different dungeon theme.
	- There's also an item drop diversity system implemented that allows different items to drop at a different chance and in varying quantities.
	- The actual item icons display for drops, and slowly float up and down and rotate in place (this actually applies to all items dropped, not just mob drops).
- **Skills**
	- Skills are the foundation for dealing damage. Both player and enemies have skills.
	- Visual processing for skill effects is present. ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/skill_visuals.png))
	- Dealing damage is functional, and both the player and enemies can inflict damage on each other.
	- Both player and enemies will die upon their health reaching zero.
	- Enemy HP bars are displayed below them. ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/enemy_hp_bars.png))
	- The player's HP regenerates by a bit after several seconds standing in the same spot, and MP regenerates by a bit every few seconds regardless.
- **NPCs**
	- NPCs are currently just rendered as a wireframe bounding box representing the space that is checked for collision against when potentially clicking on them.
	- NPC dialogues are functional and can have specialized functionality specified on either the OK or Cancel button presses on an individual dialogue basis.
- **UI windows**
	- All essentials are implemented within the core of the UI system (window dragging, universal closing, z-ordering).
	- Item tooltips are displayed on hovering over item icons in all item display UI windows. ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/item_tooltips.png))
	- **Inventory window** ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/inventory_window.png))
		- There is a 48x48px icon size, icon visuals are present, and item stacking is fully functional with stack quantity displayed.
		- There are a few different HP and MP potions included with different names, descriptions, icons, and effects.
		- Items can be clicked once to be picked up, and can be moved around different slots or even into different item-containing windows (within reason).
		- Picking up an item and then clicking on empty screen space not occupied by any windows will result in the item being dropped.
	- **Equipment window** ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/equipment_window.png))
		- There are a lot of slots and most of them aren't used, but it's very easy to adjust this as desired in code.
		- Equipped equipment items can provide stat bonuses, which are correctly added to the player's stats on equipping and removed on unequipping.
	- **Skills window** ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/skills_window.png))
		- Each skill has its icon displayed at 48x48px, along with name, level, EXP required to level up, and current EXP.
		- Skills will gain EXP upon killing enemies and will level up accordingly. EXP overflow is handled gracefully if it occurs.
		- There are a total of 6 skills included out-of-the-box.
	- **Shop window**
		- Works as a very basic concept; you can click items and they are added to your inventory.
		- No currency exists, so no cost is actually incurred out-of-the-box.
		- Not really used out-of-the-box, but exists to make it easier to improve and work with in the future if desired.
	- **World map window** ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/world_map_window.png))
		- As terrain chunks are generated and visited, the minimap will have the topmost voxel mapped on the minimap.
		- Clicking and dragging around the minimap allows you to shift the section of the world that you're viewing, displayed at the bottom.
		- Players and portals are displayed as colored squares at their respective positions on the map.
		- Every currently loaded portal is listed on the right of the map view area, along with its voxel position.
		- Portal names highlight on mouse over and are clickable to shift the map view to be centered on their location.
	- **Bank window** ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/bank_window.png))
		- Banks are spawned in every town area that specifies they should be (this is only the core town set located before the land ownership border).
		- Items can be picked up from the bank window and placed into the inventory window.
		- Bank items can be moved around and swapped within the bank window between bank slots.
		- Bank slot expansion is very trivial to do in code and the window is already designed to automatically adjust accordingly.
	- **Crafting window** ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/crafting_window.png))
		- Items can be picked up from the inventory and placed into the crafting window.
		- Items can be picked up and shifted between slots in the crafting window.
		- The resulting crafted item can be clicked to add it to the inventory (will fail if inventory is full).
	- **Keybinding window** ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/keybinding_window.png))
		- There is a core set of keybinding actions defined in code that are displayed in the bottom area, which are drawn darker if already bound, and can be picked up from the bottom area and placed onto the key slots on the window if not already bound.
		- Skills can be picked up by clicking their icon in the Skills window and bound to a key by clicking on the desired key slot in the keybinding window.
		- Items can be bound similarly to skills, by picking them up from the inventory window.
		- Base actions, skills, and items bound to a key with something already bound to it will have whatever was previously bound overwritten.
		- Already bound keys can be clicked to pick up whatever is bound to them, and it can then be moved around and placed on a different key with ease.
- **Voxel engine core**
	- Terrain is split up into 16x16x16 voxel chunks. This can easily be adjusted if desired.
	- Volume surface extraction is multithreaded. The number of simultaneous extraction threads is defaulted to 4, but it can very easily be modified.
	- Voxel terrain physics are present and hand-crafted (no physics library used). It's buggy with Y values less than 0, and it's possible to go through voxels if moving too fast, but otherwise it works fine. Both of these issues wouldn't be very difficult to fix if you wanted.
	- Land ownership is a specialized process.
		- There are land ownership and wilderness region boundaries, which are drawn to make them very clear.
		- Ownership has a time duration when claiming a chunk, and it can expire.
		- You can extend the duration of ownership of a piece of land by using the chunk claimer on a chunk you already own.
		- Ownership that has expired will not be lost until the owner unloads the chunk (walks too far away), if they are online and nearby at the moment it expires. This is accounted for in the duration that the extension occurs for, and will be added on to the total ownership time as a grace period, if the chunk is reclaimed before it's unloaded and ownership is completely lost.
		- There is a simple information display that shows who (if anyone) owns a chunk, and the remaining duration of their ownership. ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/chunk_owner_info.png))
	- Voxel placing & destroying is fully functional, but can only be done on chunks that are either owned or outside of the wilderness boundary. This is an intentional restriction.
- **Terrain generation and interaction mechanics**
	- Every chunk is only loaded on an as-needed basis and won't be loaded unless you're nearby it, unless it's specifically coded to do so for a desired reason (for example to determine the appropriate height to place a portal at).
	- There are currently 3 main factors that affect the terrain generation.
		- Biomes, which integrate the Perlin noise algorithm with variance in how it's used.
		- Dungeons, which don't use the noise algorithm or biomes.
		- Trees are randomly generated and periodically placed.
	- Relatively simple maze generation is used as the foundation for generating dungeon terrain. ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/dungeon_maze.png))
		- The boundary for dungeons is displayed. It's blue when you haven't entered it, and turns red if you do, until you beat the dungeon or die. Upon entering the boundaries of a dungeon, this movement restriction within bounds is placed upon the player, and the wave system is initialized and enemies will spawn and start pathing towards you.
		- For testing purposes, I left the walls/terrain that shouldn't be accessible in the dungeons as simply slightly elevated and darker colored ground which can be jumped on to quickly move around. Enemies will pathfind to the last best position within the valid movement bounds you should be restricted to, so to make dungeons work fully as intended, these regions of terrain should be actually blocked off from being walked over. This would be very simple to do whenever it's desired.
		- There are 3 different dungeon themes. ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/dungeon_themes.png))
		- Dungeon difficulty is increased the further the player ventures out into the dangerous wild (outer wilderness area).
		- Completed wilderness dungeon terrain is periodically regenerated after prolonged chunk inactivity (meaning nobody has been nearby for a while).
	- Tree generation is applied both in dungeons and globally out-of-the-box.
		- There are a few different types of trees that can be generated, which is reflected in the color of their trunk and leaves, and there can also be different types/sizes of the leaves' structure.
		- The overall size of the leaves' area, along with the height of the trunk, are all randomly generated in realtime upon chunk load.
	- A simple implementation of biomes is included. ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/biomes.png))
	- Outside of the core town area, the buildable terrain boundary starts, and beyond that, the wilderness region boundaries are present.
		- The core town area where new players would spawn cannot be built on/modified, so if this concept is taken into a multiplayer scenario, the game is more friendly and forgiving to new players and they don't spawn in some chaotic terrain.
		- The idea for the core town area is that there's a training ground area associated with each one, perhaps with some kind of training tower that incrementally goes up in difficulty and reward, and each town has its own theme and perhaps intended difficulty scaling as well. I was really focused on getting many other important things to work, so since I saw this as more of a "polish" type thing (something you could do later on to take the product from an alpha to beta quality), I didn't put any energy into it beyond the base concept of how I imagined it'd work.
		- The past the buildable terrain boundary, before the wilderness boundary, there are no enemy spawns, and terrain can be built on, but chunks must be taken ownership of beforehand (this can be done by standing on a chunk and using the "Chunk Claimer" item and pressing OK on the dialogue that comes up).
		- The idea is that past the wilderness boundary, PVP should be enabled. I didn't get to this, since it would've been useless anyways as things are, as I haven't implemented multiplayer.
		- Past the initial wilderness boundary, the outer boundary is what I call the "dangerous wild", and past this point, all items in the player's inventory, along with anything they have equipped, should all be dropped upon death. This, I've already implemented.
		- Throughout all of the wilderness, dungeons are randomly periodically spawned. This happens using realtime calculations as new potential dungeon spawn chunks come within closer range to the player.
	- Portals are spawned near towns and change color when you're close enough to use them. ([Screenshot](https://raw.githubusercontent.com/EmergentSpark/wings/master/screenshots/portal_colors.png))
		- Clicking on a portal displayed on the world map window (blue square if not near any portals, green square if near a portal and therefore can warp), if close enough to a portal to warp to others, will bring up a dialogue window that will warp you to the target portal if you hit OK.
- **Configuration/progress loading is present but has been disabled due to it needing some upgrades to work with the latest infrastructure.**
	- Even though it's disabled, most of it is still functional if it were to be enabled. It's pretty much just loading up inventory/equipment items that needs a little tweaking to work right, and it's not really much work even in that regard either.
	- It's convenient for testing purposes to initialize all the default values for things anyways, so it's probably better this way anyways during the alpha phase of development that this codebase comes at out-of-the-box.
	- Saving still works. You'll notice a settings.ini being generated if you gracefully exit (hit ESC).

# Technical Breakdown
- **Project configuration files are supplied for Visual Studio 2019 and tested on a Windows 10 machine.**
	- You shouldn't encounter any problems at all compiling in this environment. No errors, not even any warnings.
	- Modern but not cutting-edge C++ standards are used, along with cross-platform libraries and no platform-specific APIs, so other compiler environments, and even in other operating systems, shouldn't be difficult to setup.
	- I didn't get around to it due to time restrictions and priorities, but I was planning to use stb_image for texture loading and SQLite for chunk and detailed data storage.
- **Both Debug and Release versions link against Release library binaries.**
	- Only 32-bit Windows binaries are included.
- **No external resource dependencies.**
	- All graphical resources used are hardcoded and assembled during runtime and consist of generated geometry, both for 2D and 3D drawing. This was done for simplicity.
	- If you look through the source a bit, it's quite straightforward how you can integrate model and texture loading from external resources into the existing interface. Drawing enemies is done through a single function, and drawing icons for items and skills is done through overloading pure virtual class functions. A 3D model loading library like assimp combined with a 2D texture loading library like stb_image should work perfectly fine.
- **Assumes 1920 x 1080 resolution.**
	- Due to time constraints, I didn't get around to making a graphics settings window. Lower resolutions may not be able to fit the entirety of the window on screen at its default size, and larger or smaller resolutions may affect the mouse auto-centering functionality for the camera lock.
	- Window initialization happens in main() and the mouse movement for the camera locking happens in one function in the camera management, both pretty easy to tweak as desired. The UI already does some adjustments for some things based on window size. It's pretty straightforward and easy enough to implement resolution flexibility.
- **Uses OpenGL 1.1 so would gain a considerable performance boost if upgraded to more modern pipeline code.**
	- The low graphics quality facilitates high enough framerates for development with the OpenGL 1.1 API even on quite low-end machines, so chances are that framerate won't really be an issue for you anyways, at least out-of-the-box, unless you have a really cheap and/or old machine.
	- All rendering code outside of EntryPoint.cpp exclusively utilizes the interface exposed in Renderer.h to draw things. Code in EntryPoint.cpp may use the Renderer class or low-level OpenGL APIs.
	- GLEW is already included in the project, although unused. You can just #include it whenever you're ready to start using it, preferrably exclusively in Renderer.cpp (and maybe some future ShaderProgram.cpp and VertexBuffer.cpp files or something), with a unified and consistent encapsulated usage pre-established across the codebase beforehand as mentioned.
	- I'd strongly suggest doing more comprehensive cleanup and organization of the code, expanding upon and making more complete usage of the Renderer class for all instances of rendering-related functionality, before even thinking of messing with upgrading the pipeline usage to OpenGL 3.3+ with shaders and vertex buffers and all that.
- **Most of the code was initially developed (for speed and convenience) and remains in the large EntryPoint.cpp**
	- Pretty much all of this code was written in only about 2 months (started in June and stopped at the end of August, but was busy with other things and pretty much didn't touch this for all of July), and I was coding trying to achieve as much as possible functionality-wise in as little time as possible. To achieve this, I found it easiest not to worry too much about initially organizing everything with perfect encapsulation and scoping and proper practices, and having it all in one file to quickly collapse and expand code regions was just the fastest and easiest way to go for me.
	- Visual Studio has an extremely annoying long-standing bug where it'll randomly auto-expand (sometimes extremely large) code regions, usually while you're typing just about anywhere in a file, especially for larger files. CTRL + M + A auto collapses all the code in the whole file you have open, which is extremely convenient to be able to glance over all the neatly organized regions that I've made to make working with this single large entry point source file a lot more bearable.
- **Graphics quality and resource diversity in general are massively lacking for a production quality product.**
	- If you're planning to make use of this, I'd strongly suggest adding a lot of useful functionality, upgrading things, and working on adding a very base content pool and improving the overall gameplay flow before caring about the graphics quality or doing really any kind of visual upgrades. In a good game, gameplay should ALWAYS take priority over graphics, and there's a lot of room for a lot of improvement from a functionality and gameplay flow perspective that this source could use, before the graphics quality should even matter in terms of it having wide scope appeal to potentially dominate the MMO market.
	- It'd take even me at least 2 or 3 months just to upgrade, add, test, and balance new features and the base content pool that defines the gameplay flow, and I'm the one with the original vision that clearly has the skills necessary and the knowledge of how every single line of code in the codebase works. This means it'll probably take you longer than that (unless you're a very experienced game developer with insane drive for professional success and/or have a team) to turn this into something that could be considered even just an early beta product, so keep in mind that as considerably helpful as this release could be to you, you shouldn't expect to just need to slap on a couple finishing touches and be good kind of thing. That wasn't specifically my intent; it's just the reality of what I could achieve given the limited time that I decided I'd spend on this.
	- I knew, before I ever wrote a single line of this project, that I was probably going to have to release it all for free to the public and completely abandon all my constructive future prospects in life despite all my potential for game development and programming, so I wasn't exactly prioritizing getting high-quality graphical resources or building a diverse content pool. I wanted to make something that demonstrates my skill and potential that I'm clearly abandoning, for reasons you can learn about if you care enough to keep reading, and I do believe I've achieved that goal with this.

# About
This is the entire source and project files for a game I developed that is ultimately a culmination of over 16 years of programming experience combined with a few years of game development experience. It came after much trial and error, failing many times, a lot of blood, sweat, and tears, and was what I would've at one point considered my life's work.

I'm well aware that this source is quite valuable. If I wanted to keep improving everything, cleaning up the code and refining the functionality and gameplay flow, I feel quite certain I'd end up with a product that has considerable potential at success in the gaming industry. Instead of releasing it for free like this and just giving up on it, I could've chosen to keep working on it and pursue commercializing it and eventually just managing its growth and probably making a decent living off it. On top of that, I could've at least eventually possibly sold it for a lot of money; I wouldn't have ever done this anyways though, because I don't value money so much, and how rewarding it'd feel to know I made a great game and how fulfilling running it would be for me would well outweigh any amount of money I could ever be offered to sell it.

What I'm saying is that my decision to cease any further development on it and instead choosing to release it as open source to the public isn't out of thinking that it's trash, nor that my skills or potential are trash. I know the future prospects for continuing to develop this and eventually trying to commercialize it are quite good; I most likely could've quite easily not only made a living but acquired considerable wealth through continued efforts on it.

I'm choosing to release this source because these future prospects hold absolutely no meaning to me. All possible constructive future prospects in my life, in this field or otherwise, all hold absolutely no meaning to me any longer. I have become so thoroughly disillusioned by humanity, and so brutally deprived of any possibility of ever achieving a truly fulfilling happiness in my life, that confusing and scaring people by casually throwing away any and all constructive future prospects for myself, in favor of meditating in isolation using quantum energy harvesting meditation techniques to strengthen myself to the point that I can become an extremely brutal and cruel dictator of the world, is the only thing that I find any real meaning and purpose in, and I feel absolutely unwaveringly driven to do so without the slightest bit of hesitation or doubt. This release is irrefutable proof of this.

I don't care who you are, and I don't care whether you know of me or the circumstances of my life that have lead to things ending up how they have for this world or not, but if you're reading this, I want you to know that you're part of the problem regardless, and you will undoubtedly feel the consequences of my solution, sooner or later.

I wrote a ~500 page book explaining in great depth many things, going into considerable detail about the field of philopsychology, which to any reasonably intelligent and unbiased individual, should make it extremely clear as to why I've chosen to release this source. The contents of this book can be found both in the The_Black_Book.html in the project, as well as at http://theblackbook.cc/

The initial version of this book was officially released to the public towards the very end of 2019, and is the reason why the COVID-19 pandemic started very early in the year of 2020. It also landed me criminal charges, a court case, and a 60-day psychological evaluation in a mental hospital. By the time I was done with the doctors evaluating me, they told me they were honored to have met me, greatly appreciated my openness and cooperation in elaborating in such detail about my life and knowledge, wished me the best of luck in achieving the goal I was after, and let me leave without any resistance.

I have failed to achieve that goal, through no lack of mental fortitude and good intentions on my part, and thus have become absolutely certain that I will never desire to be a positive, constructive, good person ever again for the rest of my existence.

Because I've been so deeply tormented by my extremely unfortunate life circumstances, I ended up even writing a fairly large final statement on top of this huge book, in my last attempt to make things start heading in a positive direction. That statement has also been included in the repository as Final_Statement.html and can also be viewed at http://theblackbook.cc/?title=Final_Statement and even despite my continued extensive efforts to the point of writing this statement, it seems I'm simply destined to head down an extremely dark path in life. I've never been the type to run from reality, no matter how painful or dark it may be, and I'm not starting now, nor will I ever in the future.

Whoever you are, I don't give a flying fuck what you do with this source. You're royally fucked anyways, along with the rest of this shitty planet.

To those who relentlessly used and abused me, hated me to the ground, and facilitated others doing the same...

I will NEVER forgive or forget you.

I'm following through on EVERYTHING I've said.

Time will tell whose hatred is stronger.