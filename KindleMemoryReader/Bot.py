import discord
import asyncio

client = discord.Client()

username = input("Email: ")
password = input("Password: ")

@client.event
async def on_ready():
    print("Logged in as: ")
    print(client.user.name)
    print(client.user.id)
    print("-----")

    running  = True

    while(running):
        with open ('pagenumber.txt', 'r') as f:
            read_data = f.read()
            loc, total_loc, book_title = read_data.split(":")
            percentage = int(100 * (float(loc) / float(total_loc)))
            status_update = "Reading - " + str(percentage) + "% - " + book_title

            new_game = discord.Game(name=status_update, type=0)
            print(new_game)
            
            try:
                await client.change_presence(game=discord.Game(name=status_update, type=0))
            except:
                print("fuck you")
                
            await asyncio.sleep(120)
            print("done")
            

client.run(username, password)
