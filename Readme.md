Changes and fixes
- Model Switching
- Reconnector
- White disc
- Use key
- DM Double Kill
- DM Spectate with disc bug && disconnect
- Arena Floating
- Arena Spectate
- 2v2 White Disc and Scoreboard
- Spawn Protection
- Lose powerups after death

Gameplay Modes
- Team Deathmatch
- Last Man Standing (coming soon)
- Tournament Mode
- Decap Only Mode

In-game commands
- Respawning
- Switching Models
- Switching teams


Model Switching
>>desnotes(evil_admin) added the ability to switch models in-game with the command model [modelname]. Ex. model female

Reconnector
>>.ASM fixed a bug in the arena system where a reconnecting player can jump ahead of the queue and play before already connected players.

White disc
>>Miagi fixed a bug in arena where the player is not assigned a team and only has one disc that is white when thrown.
>>.ASM initially created a temporary fix.

Use key
>>desnotes(evil_admin) and .ASM fixed a bug where pressing a key bound to "+use" enabled a player to not be pushed by a push disc.

DM Double Kill
>>.ASM and netniV fixed a bug in deathmatch that stopped a player from getting killed on respawn if someone hit the decap'd body with a push disc.

DM Spectate with disc bug and disconnect
>>Miagi fixed a bug in deathmatch that stopped players from exploiting a bug which allowed them to go into spectate with their discs floating around them.
>>Also fixed discs getting left behind when a player disconnects

Arena Floating
>>Miagi fixed a bug in arena that enabled an opponent to "float" in the match.

Arena Spectate
>>Miagi fixed a bug in arena that enabled a spectator to "float" and kill people in other matches.

2v2 White Disc and Scoreboard
>>Miagi allowed/fixed the use of teamplay in arena and fixed the scoreboard to keep teams together.

Spawn Protection
>>Wha? added spawn protection to deathmatch so that players can't get killed or kill others for a set period of time after spawning.

Lose powerups after death
>>Wha? stopped players from keeping powerups after dying to improve gameplay.

Team Deathmatch
>>Miagi and Wha? added a team deathmatch gameplay mode of red vs blue.
>>Players can switch teams with the team [red or blue] command.
>>Connecting players are assigned the team with the fewest players.

Last Man Standing (coming soon)
>>Miagi is working on a gameplay mode similiar to TS The One Mode, but the opposite.
>>Concept is a deathmatch style game where the goal is to kill each other until 1 man is left standing.

Tournament Mode
>>Miagi and Wha? added a cvar of "mp_tournament" "0 or 1" that lets a server admin disable powerups for all maps.

Decap Only Mode
>>Miagi added a cvar of "mp_decaponly" "0 or 1" that lets a server admin disable the use of push discs.

Respawning
>>ready - Will enable a player to respawn in deathmatch after going into spectate.

Switching Models
>>model [modelname] - Will allow a player to switch models provided that the model name ends with "male".

Switching teams
>>team [red or blue] - Will allow a player to switch teams in deathmatch teamplay.
