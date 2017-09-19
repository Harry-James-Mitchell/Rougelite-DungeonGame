This project is optimized to run on Linux.

To run this project type make into the terminal and then ./rlg327 
Make sure you put the monster_desc file I have included in your home directory when testing this project.
this project works best on one of the linux virtual machines instead of pyrite.

ShopKeepers can appear in the dungeon and are represented by a Green $. When the PC attempts 
to move onto the same sapce as a shopkeeper a shop menu opens up that contains three random 
items. The PC can spend their money to buy these items and they will be put into their 
inventory. A shopkeeper disapeers after the PC talks to them for the first time. 
This is intentional for balance reasons I will add later.

Pressing the ! key during the PC's turn will display all relavent information about them.

Expunging items will give the PC money equal to half of the value the item had. This is
the main way for the PC to generate money.

The PC's base damage is now 10+1d4.

The weight of items now matters. If the PC has equiped more than 25 weight worth of items
then their speed is halved. They still get all of the speed bonus of items they have equiped.

Monster deal damage to the PC equal to their damage - PC's defence.

Characters now recover 10% of their max health every 5 turns. The player can veiw how much
time they need to wait to heal by pressing the ! key on their turn.