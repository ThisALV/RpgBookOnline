return {
    [0] = {
        {
            "Title", {
                text="This is a sample RPG for github tutorial."
            }
        },
        {
            "Goto", {
                scene=1
            }
        }
    },
    [1] = {
        {
            "Note", {
                text="Entering into a room, you see a little chest in the corner."
            }
        },
        {
            "Text", {
                text="The group's chief opens the chest...",
                wait=true
            }
        },
        {
            "EventTo", {
                target="leader", effect="unlock_chest"
            }
        },
        {
            "Alert", {
                text="He just found a Magical potion * 1 !"
            }
        },
        {
            "YesOrNoDecision", {
                question="Do you want to drink it ?",
                target="leader",
                yes=2, no=30
            }
        }
    },
    [2] = {
        {
            "Text", {
                text="He decided to drink that, then he suffocate."
            }
        },
        {
            "EventTo", {
                target="leader", effect="death"
            }
        },
        {
            "Text", {
                text="Unhealthy vapors begin to invade the room and hurt you..."
            }
        },
        {
            "EventTo", {
                target="all", effect="dmg"
            }
        },
        {
            "Text", {
                text="On the emergency, you run for the only exit of this room."
            }
        },
        {
            "Goto", {
                scene=3
            }
        }
    },
    [30] = {
        {
            "Text", {
                text="You see a door at the opposed to the one you're coming from and decide to enter it."
            }
        },
        {
            "Goto", {
                scene=4
            }
        }
    },
    [3] = {
        {
            "Checkpoint", {
                name="hall", scene=4
            }
        },
        {
            "Goto", {
                scene=4
            }
        }
    },
    [4] = {
        {
            "Text", {
                text="You arrived into a massive hall. Into the wall in front of you, there are 3 doors waiting for you."
            }
        },
        {
            "PathChoice", {
                message="What do you choose to do next ?",
                paths={
                    [5]="Stay here",
                    [6]="Enter left door",
                    [7]="Enter middle door",
                    [8]="Enter right door"
                },
                wait=false
            }
        }
    },
    [5] = {
        {
            "SurpriseAttack", {
                enemiesGroup="Hand which emerges from the roof"
            }
        },
        {
            "DrinkMagicPotion", {
                max=10
            }
        },
        {
            "Alert", {
                text="Even if you're still dizzy, the hand attacks you again !"
            }
        },
        {
            "EventTo", {
                target="all", effect="dmg"
            }
        },
        {
            "Text", {
                text="You survived this attack, END.",
                wait=true
            }
        }
    },
    [6] = {
        {
            "Note", {
                text="Opening left door..."
            }
        },
        {
            "Goto", {
                scene=9
            }
        }
    },
    [7] = {
        {
            "Note", {
                text="Opening middle door..."
            }
        },
        {
            "Goto", {
                scene=9
            }
        }
    },
    [8] = {
        {
            "Note", {
                text="Opening right door..."
            }
        },
        {
            "Goto", {
                scene=9
            }
        }
    },
    [9] = {
        {
            "Text", {
                text="All doors lead to the same room, END.",
                wait=true
            }
        }
    }
}
