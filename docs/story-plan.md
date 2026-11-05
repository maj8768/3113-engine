# Wizard game: story and production plan

This is a proposed playable script and implementation sequence. Dialogue, objectives, and timings below are a first draft ready to prototype; they are not implemented gameplay.

## 1. Story contract

The player starts as a driver traveling to Unspecified City, becomes the man living beneath the bridge after a collision, and eventually becomes a driver again by taking another car. The opening and ending frame the same cycle from opposite positions. Preserve the unexplained change in perspective; no narrator needs to establish reincarnation, time travel, or a body-swap rule.

The protagonist increasingly believes he is a wizard. A stick becomes a wand, a stolen vacuum becomes a magical artifact, and stopping a car becomes a spellcasting victory. The player sees his interpretation through effects, sound, animation, and objective wording. Physical surroundings remain ordinary. Avoid a final speech explaining that the magic was imaginary: small contradictions should carry that realization.

The legal-system satire centers on a specific fictional city: enforcement is immediate, processing is elaborate, detention accomplishes nothing, and the protagonist returns to finish the theft. The release deliberately contradicts the arrest. Do not resolve the contradiction with a technicality that makes the system sensible. The drone sincerely considers every step a successful outcome.

The contact is saved as **M**. His identity stays unexplained. He speaks casually, endorses increasingly bad decisions, and never supplies meaningful help. The protagonist's replies become more grandiose over time. All reply choices converge on the same required objective, but they change M's immediate response and occasionally a later callback.

Target length: approximately two hours on a first playthrough. Structure: playable prologue, three main levels, and short transitions between levels. The original linear outline is a 20–30-minute story spine; reaching two hours still requires substantial playable expansion outside the courtroom. The courtroom below replaces the interview-room proposal and targets 12–18 minutes through participation, evidence inspection, and dialogue rather than forced waiting.

## 2. Player controls and presentation

These are proposed controls. Keep the existing F pickup interaction and 1 fireball binding; contextual dispatch will need to prevent overlapping actions.

| Context | Input | Behavior |
| --- | --- | --- |
| On foot | WASD / mouse | Move / look. Normal human walking speed; Shift jogs. |
| On foot | Space | Small jump. No platforming required to finish. |
| Interaction | F | Pick up or place an item, use a door, enter a car, or interact with an objective. One highlighted action wins each frame. |
| Holding an item | Hold F while looking away from interactables | Drop it after a short visible hold indicator. Tapping F on a valid target performs its contextual action. |
| Phone | Tab | Open or close the phone when available. |
| Phone / courtroom dialogue | Mouse or arrows + Enter | Choose and confirm an answer. Number keys may select answers only while this interface owns input. |
| Wand | 1 | Cast the fireball presentation when enabled by story state. |
| Shopping cart | F | Begin or stop pushing. WASD controls movement; mouse steers the view. |
| Car | W/S, A/D | Accelerate/brake or reverse, steer. Mouse looks around within seated limits. |
| Menus | Escape | Pause, settings, restart checkpoint. |

- Phone conversations pause movement and nearby gameplay. Courtroom choices hold the next spoken line until the player answers, while idle animations continue. Stop the player safely before opening either interface. Urgent texts received on the road wait until the player is safe.
- Closed unread messages remain available. No required information disappears because a notification expired.
- The phone occupies its own UI layer. Opening it temporarily lowers the held object; it does not throw the wand onto the ground.
- Only one main objective is visible. Use short notifications for jokes, and put longer material in the phone, case packet, or spoken dialogue.
- Cutscenes normally last 5–15 seconds. Preserve mouse look where possible. Restrict movement only during actions that need exact staging.
- Subtitles show every spoken line, including overlapping objections and dialogue over black. Provide separate music, dialogue, and effect volume controls, plus reduced camera motion and reduced distortion settings.
- Reserve broad visual haze for the final level. The player must always be able to read signs, text, interactables, and the road.

## 3. World layout and movement route

Build a compact reusable map rather than three unrelated open worlds.

```text
Gas station → approach road → BRIDGE → town road → hardware store
                               |
                     footpath / embankment
                               |
                       tent beneath bridge

Courthouse holding corridor → courtroom: separate interiors loaded during blackout.
```

The bridge is the visual anchor. A warm tent light, visible footpath, and shelter draw the player toward it. Closed businesses, fences, highway noise, drainage channels, and distant lights make other directions uninviting. Avoid unexplained invisible walls.

The store is roughly a minute's walk from the bridge. Its entrance and illuminated sign are visible from the town approach. Inside: cart bay, central aisle, cleaning-appliance aisle, checkout lane, exit. The route must work with a cart without demanding precise physics.

Place the tent safely above the waterline. Give the player an obvious walking route between the tent and the road; never require climbing the bridge supports.

The town exists throughout. During the first evening, darkness, fog, and the direction of the player's attention obscure it. Morning reveals the route; buildings do not visibly spawn in.

## 4. Playable prologue — A New Life

**Purpose:** establish ordinary reality, teach interaction and driving, then reverse the player's role.

**Starting position:** first-person, standing beside a parked car at a gas station, facing the attached nozzle. Late afternoon. A road sign reads **UNSPECIFIED CITY — A FRESH START**.

### P01 — Finish fueling

1. Objective: **Return the nozzle.**
2. Player approaches and presses F. A short animation removes and returns it; do not make the player simulate the hose.
3. Pump display: **PAYMENT ACCEPTED. FINANCIAL SECURITY NOT INCLUDED.**
4. Objective changes to **Get in the car.** The driver's door receives the interaction highlight.

### P02 — Leave the station

1. F at the driver's door seats the player with a brief camera blend.
2. Prompt: **F — Start engine.**
3. Starting the engine turns on the radio. Objective: **Drive to Unspecified City.**
4. The station exit and road geometry guide the turn. Prevent an accidental wrong-way departure with ordinary barriers and clear signage.

### P03 — The drive

Allow approximately two minutes of actual forward driving before the encounter. Pausing or sitting still does not advance this requirement. A normal-speed run should reach the bridge as the time requirement completes; use a short concealed approach section if necessary. Trigger the encounter only after both the drive requirement and the bridge approach marker are reached.

Use an original or licensed music track. Give the player enough steering to feel responsible, but use a forgiving road and no unrelated traffic challenges. On leaving the road, stop safely or offer a quick reset to the last road marker.

Optional radio lines, spaced between music:

> “Unspecified City: your future is waiting. Terms may change without notice.”

> “Traffic is moving smoothly. Please disregard anything beneath the bridges.”

### P04 — Collision and perspective change

1. At the bridge approach, briefly blend steering and speed into a scripted path. Maintain limited mouse look.
2. A man in a distinctive coat steps from the bridge-side path into the lane and raises a stick. From the driver's seat, the motion reads as sudden and irrational.
3. Brake sound, impact, hard cut to black. No graphic injury shot.
4. Fade up at low camera height beside the bridge path. The player's sleeves now match the man's coat.
5. The opening car drives away. Its driver is an indistinct silhouette. The player's old viewpoint is now literally receding from them.
6. A phone rings. Full walking control returns once the player stands.

Do not put a successful evasive-driving challenge inside a mandatory story collision. The scripted approach begins before the man steps out, with consistent audiovisual staging.

**Checkpoint:** standing beside the bridge, before opening the first conversation.

## 5. Level one — The Errand

**Purpose:** establish the bridge as home, introduce M, acquire the stick and vacuum, and end in arrest.

### L1.01 — An expensive phone

Objective: **Check your phone.**

Show a pristine luxury phone against dirty sleeves. An optional notification reads **Device payment 19 of 48 is due.** The phone rings, then the call drops before anyone speaks. M immediately texts; the conversation itself uses text choices.

**Exchange 1**

M: “you still getting the gas?”

| Player response | M replies |
| --- | --- |
| “That was the plan.” | “good. plans are important.” |
| “I think I just got hit by a car.” | “rough start. how's the plan though?” |
| “Who is this?” | “M. same as yesterday.” |

**Exchange 2**

M: “your lungs are shot. stop trying to do everything yourself. get a vacuum.”

| Player response | M replies |
| --- | --- |
| “A vacuum cleaner?” | “finally, some proper equipment.” |
| “I don't have money.” | “i said get one.” |
| “This sounds like a terrible idea.” | “that's why we're planning ahead.” |

**Exchange 3**

M: “hardware place in town. tomorrow. bring something that makes you look serious.”

| Player response | M replies |
| --- | --- |
| “I'll find something.” | “resourceful. that's what i like about you.” |
| “I have a stick.” | “then you're already equipped.” |
| “I'm not threatening anyone.” | “then look serious politely.” |

The conversation completes after the third response. Closing the phone earlier preserves the current exchange; the sleep objective waits for completion.

### L1.02 — Home beneath the bridge

While the phone is raised, sunset advances through lighting and ambient sound. On closing it, the bridge is dark and the tent light is conspicuous.

Notification: **It is time to sleep.** Objective: **Find your tent.**

Movement route: roadside → fenced footpath → embankment → tent. Near the tent lie a blanket, personal belongings, and the same distinctive stick seen during the collision.

The player may inspect the stick now: **A good stick.** Picking it up is optional tonight. If left behind, it remains beside the tent in the morning.

F at the tent entrance plays a short crouch-and-enter camera movement. Tent fabric fills the screen, city sound continues, then fades to morning. This is an overnight transition within level one.

### L1.03 — Morning and the wand

Wake at the tent entrance. Notification: **It is time.** Objective: **Go to the hardware store.**

New text from M: “don't forget your stick. confidence is half of everything.”

| Player response | M replies |
| --- | --- |
| “It's just a stick.” | “not with that attitude.” |
| “You mean my wand.” | “yeah. that.” |
| “What's the other half?” | “equipment. we covered this.” |

Objective briefly becomes **Take your stick** if it has not been collected. Once picked up, its interaction label becomes **Wand**. There is no modal announcement that the character is delusional.

Allow one optional practice cast toward a safe empty area: a small subjective spark, followed by an ordinary stick swish. No combat tutorial or damage requirement. The later car encounter receives the first full fireball spectacle.

Climb the footpath. Morning reveals the town across the bridge. A store sign and sidewalk lead the player to the entrance.

### L1.04 — Shopping

1. Enter the hardware store. Door chime; harsh fluorescent lighting.
2. Store speaker: “Welcome. Your needs are our inventory.”
3. Objective: **Get a shopping cart.** F attaches the player to a cart at the entrance. The wand is temporarily stowed visibly in the cart.
4. Objective: **Find a vacuum.** Follow overhead **CLEANING / HOME IMPROVEMENT** signs. Put two or three optional dead-end product displays along the route, without making it a maze.
5. Target product: **HOMEWIZARD 3000 — TAKE CONTROL OF YOUR ENVIRONMENT.**
6. F places the boxed vacuum in the cart with a short animation. Do not require balancing a loose physics object.
7. Objective: **Leave with the vacuum.** Guide the cart through the checkout lane.

Optional shelf jokes: **PERSONAL RESPONSIBILITY — SOLD SEPARATELY** and **EVERYDAY ESSENTIALS — FINANCING AVAILABLE**. Use a few readable jokes rather than filling every surface.

### L1.05 — The robbery

Crossing the checkout approach starts the exchange. The cashier stays behind the counter; the player retains look control. The cart pauses on a clear floor marker.

Cashier: “That's one hundred and eighty-nine ninety-nine.”

Player chooses:

1. “I have no money.”
2. “This is a requisition by order of the wizard.”
3. “I'm taking this.”

Cashier, to any response: “Then I need you to leave the vacuum here.”

Objective: **Raise your wand.** F retrieves it from the cart; holding the highlighted interaction raises it toward the counter. The protagonist says, “Stand aside.” This explicitly makes the theft an attempted stick-assisted robbery, rather than merely walking past a cashier.

Cashier: “Please don't point that at me.”

A brief glow and magical audio swell occur in the player's perception. A loose paper flutters; nothing visibly burns or explodes. The cashier steps away from the lane and presses a button below the counter.

Cashier: “I've reported it. I'm not getting hurt over a vacuum.”

Resume cart control. Objective: **Leave the store.** The wand stows again while pushing. Doors open automatically. Security gate beeps once, then plays: “Thank you for shopping with us.”

### L1.06 — Drone interception / cutscene into level two

Start a five-second timer once the loaded cart clears the exterior exit marker. Do not run this timer while paused or in a menu. The drone approaches from the street and moves into view in front of the cart; it should not depend on the player looking in a precise direction.

The cart brakes; the drone blocks the path. Limited mouse look remains.

Drone:

> “Stop. You are being detained for theft, threatening behavior, and failure to complete checkout.”

> “Remain calm. Compliance is mandatory.”

The drone deploys its taser. A short jolt, falling camera movement, muffled sound, and gradual fade begin. Reduced-motion mode uses a steady fade.

Drone, continuing over black:

> “Your safety is our highest priority. You will now be delivered to the court for further processing.”

Load the courthouse holding corridor. Save the vacuum and wand as story-owned possessions so ordinary item cleanup cannot delete them.

**Checkpoint:** standing in the holding corridor before the courtroom doors open.

## 6. Level two — Due Process

**Purpose:** participate in a formally staged courtroom hearing while every available contribution makes the defense worse, then be released without consequence and return to complete the theft.

This replaces the questionnaire and the previously suggested document-fetching bureaucracy. The humor comes from the contrast between recognizable courtroom behavior and the protagonist's terrible contributions. The judge and attorneys mostly speak plainly and take the proceeding seriously; they are not all competing to deliver jokes.

The sequence is a fictional initial hearing that veers into questions about the defendant's ability to participate. Use the visual and conversational structure of a courtroom proceeding, rather than presenting this as an accurate procedural simulation. There is no jury, full trial, or acquittal. A confession in dialogue is not automatically treated as an accepted guilty plea; the judge explicitly interrupts plea-taking because the defendant cannot stay on topic. The eventual release remains the game's deliberately absurd outcome.

### L2.01 — Enter the courtroom

Fade up standing in a short holding corridor. The arrest drone hovers by the exit. A human bailiff opens the courtroom door. Objective: **Go to the defense table.**

Layout: judge on a raised bench, clerk to one side, prosecution table, defense table, witness stand, an evidence monitor, and a mostly empty public gallery. The vacuum and stick are visible on an evidence trolley. Keep the room compact so faces and evidence remain readable from the player's seat.

Required cast: judge, public defender, prosecutor, clerk/bailiff, arrest drone, and the existing cashier as a witness on the monitor. The clerk and bailiff can be one character for production scope. The cashier's testimony can be prerecorded footage rather than a second in-person performance.

Movement: walk a short escorted route to the table, then F to sit. Once seated, retain mouse look and allow inspection of the case packet, evidence screen, and attorney. Standing or sitting when directed uses F; nobody needs to maneuver precisely around chairs.

The phone is unavailable during the hearing. The wand and vacuum are evidence, not usable inventory. No spells can be cast in the room.

Public defender, quietly:

> “I'm your attorney. Answer the question you're asked. If you don't understand it, tell me.”

Player chooses:

| Response | Attorney replies |
| --- | --- |
| “I did steal it. Do we still need to do this?” | “Please don't announce that to the room.” |
| “Can I replace you with my wand?” | “No.” |
| “Before we start, what's your position on turkey burgers?” | “My position is that we shouldn't discuss them in court.” |

Clerk:

> “All rise. The court is now in session.”

Objective: **Stand.** F stands the player. The judge enters and sits.

Judge:

> “Please be seated. Call the next matter.”

F sits the player. Clerk:

> “City versus [player name]. Alleged theft and threatening conduct at the hardware store.”

Optional case-packet inspection: a receipt showing an unpaid vacuum, a still from store footage, and the arrest report. Inspecting each reveals a later evidence-specific response. Those extra responses are also terrible; inspection provides material and context rather than a secret competent defense.

**Checkpoint:** seated at the defense table, case called, first question pending. Save inspected evidence and committed answers at subsequent exchange boundaries so a reload need not replay the entire hearing.

### L2.02 — The hearing

Target 12–18 minutes including entry, inspection, optional follow-ups, and departure. Do not make every line take a fixed amount of time just to reach this target. Allow advancing a finished subtitle and a visible **Skip remaining speech** option for long selected monologues, preserving their recorded outcome. A repeat run may be substantially shorter.

#### Beat 1 — Identity

Judge:

> “State your name and occupation for the record.”

| Response | Reaction |
| --- | --- |
| “I'm the man who stole the vacuum. I'm currently unemployed.” | Attorney: “Your Honor, I ask that he be allowed to answer with his name.” |
| “I am a wizard. You may address me accordingly.” | Judge: “Your name will be sufficient.” |
| “Before I identify myself, I need to say something about turkey burgers.” | Judge: “You do not.” |

After either nonresponsive answer, the attorney supplies the name from the case packet and the clerk records it. Choosing Wizard sets **claimed wizard in court** for later callbacks. Turkey-burger selections set **burger subject introduced**.

#### Beat 2 — Charges and attempted plea

Prosecutor:

> “The allegation is that you placed a vacuum cleaner in a cart, threatened an employee with a stick, and left without paying.”

Judge:

> “Do you understand what you are accused of?”

| Response | Reaction |
| --- | --- |
| “Yes. That's exactly what I did.” | Attorney: “He's answering whether he understands. That is not a plea.” |
| “I contest the word stick. Everything else is correct.” | Prosecutor: “The distinction does not change the allegation.” |
| “I understand turkey burgers, and I think the court needs to as well.” | Judge: “The court needs you to answer this question.” |

Judge:

> “I am not accepting a plea until I am satisfied you understand this proceeding. We will clarify what happened first.”

This line keeps subsequent evidence discussion and the suitability question coherent even if the player has already confessed. Never label a confession option **Plead guilty** and then pretend a completed plea has no procedural effect.

#### Beat 3 — Evidence

Objective: **Inspect the evidence screen.** F focuses the monitor; look or directional inputs cycle a store still, the unpaid receipt, and the stick photograph. The player confirms **Finished** to return to the judge. Evidence has legible captions and no pixel-hunting requirement.

The arrest drone gives a short factual account from beside the prosecution table:

> “The subject exited with one unpaid vacuum cleaner. Store footage records the subject pointing a wooden object at an employee. I detained the subject outside.”

Judge:

> “Is there anything you wish to clarify about that account?”

| Response | Reaction |
| --- | --- |
| “It's accurate. I'd also like the vacuum back.” | Judge: “The property is not being returned to you.” |
| “It was a wand. I feel like nobody is listening.” | Attorney: “We have heard your position on the object.” |
| “A turkey burger is evidence that looking like something doesn't make you that thing.” | Prosecutor: “Is he disputing identification?” Attorney: “I don't believe so.” |

If the unpaid receipt was inspected in the case packet, add: **“The receipt proves I couldn't afford it. That's why I took it.”** Prosecutor: “Then we agree no payment was made.”

If the stick photograph was inspected, add: **“You photographed the wand at its worst angle.”** Judge: “The photograph will remain as it is.”

#### Beat 4 — The cashier

The cashier appears on the monitor. Clerk asks for a promise to tell the truth; cashier agrees. Keep the staging brief and recognizable.

Prosecutor:

> “What did the defendant do when you asked him to pay?”

Cashier:

> “He pointed the stick at me and told me to stand aside. I moved. I didn't know what he was going to do.”

The attorney asks one ordinary question: “Was anyone physically injured inside the store?” Cashier: “No. I moved out of the way.” This establishes an actual fact without erasing the threat.

The protagonist interrupts. Objective: **Choose your interruption.**

| Response | Reaction |
| --- | --- |
| “She's telling the truth. I was trying to frighten her.” | Attorney, quietly: “You are making this harder.” |
| “Ask whether she felt the magical pressure.” | Cashier: “I felt threatened by a man holding a stick.” |
| “Ask whether the employee break room serves turkey burgers.” | Cashier: “No.” |

Judge:

> “You will allow your attorney to ask the questions.”

If turkey burgers have already been mentioned, the cashier adds: “You didn't mention burgers in the store.” That gives the witness continuity without turning her into a comedian.

#### Beat 5 — The statement / turkey-burger monologue

Judge:

> “Before I decide how this matter proceeds, is there anything relevant you want your attorney to raise?”

Offer three visibly disastrous choices. No neutral **Stay silent** or **Let your attorney answer** option appears. The objective is simply **Respond**; do not label any choice as a successful strategy.

**A — Confess and announce the next theft**

> “I took the vacuum because I wanted it and couldn't pay for it. If you let me go, I'm going back for it. I don't understand what part is confusing.”

Attorney: “Please stop telling the court what you plan to do next.”

**B — Explain the turkey-burger problem**

> “A turkey burger is what happens when somebody wants credit for solving a problem they haven't solved. You order a burger. They bring you something shaped like a burger. There's a bun. There's lettuce. They put it in a little basket. Everyone's looking at the basket like the basket makes it beef.”
>
> “Then you say it's dry, and suddenly you're the problem. Did you try mustard? Did you try avocado? Why is it my job to assemble a support system for this sandwich? A hamburger doesn't need a public relations department.”
>
> “And they charge more. That's the part nobody will explain. You took away the thing I wanted, made me do extra work, and charged me for the improvement. So yes, I took the vacuum. It was the only thing in that store that was actually what it said it was.”

During the speech, allow looking around: the clerk tries to keep up, the prosecutor looks at the clock, and the attorney gradually stops taking notes. One short attempted interruption occurs after the first paragraph.

Judge: “Please explain how this relates to the allegation.”

Player chooses:

1. **Continue:** “I'm getting to the basket.” Continue with the remaining paragraphs.
2. **Connect it directly:** “It's relevant because I stole the vacuum.” End the speech with an explicit confession.
3. **Challenge the judge:** “Have you ever had a good one? Under oath?” Judge: “I am not a witness.” End the speech.

If burger dialogue was selected earlier, open with **“As I've been trying to explain—”**. Otherwise open with the first sentence above. Do not repeat the full monologue at earlier burger choices.

After the speech, judge: “The record will reflect that the defendant has not answered the question.” The confession remains in the transcript; the court's response is about his inability to stay with the proceeding, not the words disappearing.

**C — Claim magical authority**

> “I requisitioned the artifact under powers this court isn't qualified to review. Return my wand and I'll demonstrate. I'd prefer the judge to stand a little farther back.”

Judge: “There will be no demonstration.” Attorney: “And no one is returning the stick.”

Choosing C also sets **claimed wizard in court**. Every branch moves to the same suitability question, with a different reaction line and a saved transcript entry.

#### Beat 6 — Can the defendant participate?

Attorney:

> “Your Honor, I have concerns about his ability to follow this hearing and assist me. I'm asking the court to address that before proceeding further.”

Judge:

> “Do you understand what your attorney is here to do?”

| Response | Reaction |
| --- | --- |
| “Keep me out of jail even though I did it.” | Judge: “Do you understand that your answers are being recorded?” |
| “He's my apprentice. He's doing very badly.” | Attorney: “We met approximately ten minutes ago.” |
| “Ideally, explain why a turkey burger costs fourteen dollars.” | Judge: “We are not returning to the burger.” |

One final follow-up, with no time limit:

Judge: “What do you believe will happen if you leave here today?”

1. **“I'll go back and take the vacuum.”**
2. **“My powers will be restored.”**
3. **“Lunch, if anyone in this city is capable of making it.”**

Record the answer. The attorney and prosecutor have a short exchange about next steps; avoid a long off-screen conference or simulated real-time recess.

#### Beat 7 — Ruling and release

Judge:

> “I am not proceeding with this case today. I find this defendant unsuitable to stand trial. He will be released, and no further action will be taken in this matter.”

Prosecutor:

> “Your Honor, the property and the store employee—”

Judge:

> “The disposition has been entered. Call the next case.”

The clerk stamps the file. The court has not acquitted the protagonist or disproved the allegation. The disconnect is the swift release despite the evidence and his responses; the city's later notice will nevertheless tell him he did nothing wrong.

If the player chose the full confession: Attorney, quietly: “Please don't go back to the store.”

If the player chose the burger monologue: Attorney, quietly: “Next time, let me do the talking.”

If the player chose magical authority: Attorney, quietly: “Leave the stick here.”

Objective: **Leave the courtroom.** F stands the player. Walk the short route back toward the door while the next case is called. The arrest drone meets the player in the holding corridor.

Drone:

> “Your hearing is complete. Please remain still for release.”

It tases the player again; fade to black. The judge's formal release and the drone's absurd transport procedure stay distinct. The player does not fight or evade this scripted transfer.

### L2.03 — Released at the store

Fade up outside the same hardware-store entrance, now in late afternoon. The player stands safely away from traffic. The empty cart is at the entrance. The wand rests across its basket; the original boxed vacuum is back on its shelf.

Show a readable release notice requiring dismissal:

> **RELEASE NOTICE**
>
> The judge has found you unsuitable to stand trial.
>
> No further action will be taken. You did nothing wrong.
>
> You are free to go.

The last two claims deliberately clash with the evidence and the court's actual ruling. Present them as the fictional system's official notice, with no explanatory narration. The notice upgrades a procedural release into a claim of innocence; the game does not need to explain that contradiction away.

Drone, already departing:

> “This incident has been resolved. Thank you for your cooperation.”

M texts: “you get it?”

| Player response | M replies |
| --- | --- |
| “I got arrested.” | “okay but did you get it?” |
| “The judge says I did nothing wrong.” | “great. go finish.” |
| “They electrocuted me twice.” | “sounds like you're fully charged.” |

If the turkey-burger monologue was chosen, M adds: “heard you had a lot to say about lunch.” Player may answer **“They needed to hear it.”** or **“Nobody answered my question.”** M replies to either: “vacuum first.”

Objective: **Go back for the vacuum.**

### L2.04 — The second theft

Let the player repeat the physical route: collect the cart, retrieve the vacuum, pass the cashier, leave. The familiarity is part of the joke; shorten traversal through open aisles, not a teleport straight to the shelf.

At the shelf, M sends a one-line notification: “take the same one. no need to overthink this.” No response required.

Cashier, recognizing the player:

> “You're back?”

Available spoken replies:

1. “They said I did nothing wrong.”
2. “The court recognizes my authority.”
3. “I'm still taking the vacuum.”

Cashier, to all:

> “They closed the report. I'm not doing this twice.”

The cashier steps aside. If the player raises the wand again, the cashier adds: “You can put the stick down.” A second threat is optional; progress does not require repeating it.

Exit announcement: “Thank you for shopping with us.” This time no drone arrives, even after the player waits outside. The player can finish the theft freely.

The loaded-cart exit sets **vacuum acquired** and saves a checkpoint. Neither the courtroom hearing nor the arrest can trigger again.

### L2.05 — Return home / cutscene into level three

Objective: **Bring the vacuum home.** Player pushes the cart back across the town sidewalk and down a broad path beside the bridge. Check this route early with the cart dimensions; stairs cannot be the only route.

On reaching the tent, F places the vacuum beside it and parks the cart. The wand becomes available in hand again. Evening lighting deepens during the journey; night completes only when the vacuum has reached home.

M texts: “knew you had it in you.”

| Player response | M replies |
| --- | --- |
| “It's here.” | “tomorrow's the big day.” |
| “The wand worked.” | “keep it close.” |
| “I'm going to sleep.” | “rest up, wizard.” |

If **claimed wizard in court** is set, add: “told a judge you're a wizard and walked out. productive day.”

Notification: **It is time to sleep.** Objective: **Sleep in the tent.**

Entering the tent starts the level transition. Show the bridge overhead through the tent flap. Traffic noise stretches into an ominous musical tone; the familiar radio melody is barely audible. Fade out, save, then fade into morning.

**Checkpoint:** morning at the tent, vacuum beside the player, wand available, final conversation unread.

## 7. Level three — The Wizard

**Purpose:** complete the delusion, stage the opening encounter from the roadside perspective, and return the player to the driver's seat.

### L3.01 — A grand plan

Wake with mild peripheral haze, unstable distant sound, and slightly altered colors. Keep central vision and UI clear. The vacuum looks ordinary when inspected, but its objective label is now **Extraction Staff**. The stick remains visually a stick.

Objective: **Read M's message.**

**Exchange 1**

M: “you're probably going out of your mind by now. you've been talking about the gas for days.”

| Player response | M replies |
| --- | --- |
| “Everything looks wrong.” | “one thing at a time.” |
| “My powers are coming back.” | “good timing.” |
| “Then why aren't you helping?” | “i am. i'm organizing.” |

**Exchange 2**

M: “take the vacuum up to the bridge. stop a car. get what you came for, then take the car and come here.”

| Player response | M replies |
| --- | --- |
| “That's your whole plan?” | “simple plans get finished.” |
| “I'll stop it with the wand.” | “there you go.” |
| “What about the driver?” | “you're getting distracted.” |

**Exchange 3**

M: “bring some for me too.”

| Player response | M replies |
| --- | --- |
| “Of course.” | “knew i could count on you.” |
| “The wizard provides.” | “that's the spirit.” |
| “You could have come yourself.” | “and miss watching you grow?” |

Close phone. Objective: **Take your equipment to the bridge.**

M's instructions are a bad fictional plan, not an instructional minigame. Keep fuel collection and inhalant use stylized and nonspecific. Do not include real handling instructions, quantities, or equipment diagrams.

### L3.02 — Prepare at the roadside

F picks up the vacuum in a carry mode with the wand visibly strapped to it. Carrying slows the player modestly but still permits jogging. At a clear roadside marker, F sets the vacuum down and equips the wand. This avoids requiring two simultaneous held physics items.

Movement route repeats the familiar footpath, this time upward. The bridge takes on the visual importance of a wizard's tower through framing and sound, without changing its geometry.

At the waiting marker, objective: **Wait for a car.** Show **F — Step forward when ready** after the car appears far down the road. The car will not enter its encounter phase until the equipment is staged and the player commits.

A new car approaches. Reuse the opening car model, dashboard ornament, radio track, and headlight pattern. This is another encounter in the same cycle; do not introduce a time-travel explanation or claim that the two cars must be one simultaneous object.

New text from M: “make it stop.” The notification appears without opening the phone.

### L3.03 — Cast at the car

1. Player presses F to commit and walks a few steps from the waiting marker into the lane. Blend movement onto the required mark, with look control retained.
2. Frame the scene to echo the man stepping into the road during the opening.
3. Objective: **1 — Cast fireball.** The car's cinematic approach holds enough distance for the player to act. No lethal reaction timer.
4. On cast, launch the existing fireball presentation: charge, projectile, light, smoke, magical impact sound. Keep the wand visible so the player feels responsible.
5. Headlight flare, brake sound, an abrupt impact-like sound, then black. The protagonist experiences this as successfully overpowering the vehicle.
6. Fade up low beside the stopped car. It is intact, with a stressed driver still at the wheel. The vacuum is at the roadside marker within reach.

This staged spell does not use normal projectile damage to destroy or launch the car. The subjective explosion and the scripted physical outcome are separate. Preserve the ambiguity of the blackout without requiring a second literal injury.

Driver, through the window:

> “What the hell are you doing? I nearly hit you.”

If the player lingers:

> “Get out of the road. And put that stick down.”

The ordinariness of the car and the driver's reaction contradict the apparent magical victory.

### L3.04 — The artifact

Objective: **Retrieve the vacuum.** Walk to it and press F.

Objective: **Use the artifact on the stopped car.** A single contextual interaction at a broad marked area begins an abstract montage: vacuum sound, the protagonist's hands obscuring the view, jump in ambient sound, brief fade. No realistic extraction simulation.

Objective: **Complete the ritual.** F starts a brief implied inhalant-use beat, with no detailed handling animation. The character's subjective haze fades, the camera steadies, and magical ambience becomes a triumphant variation of the radio melody. Treat this as his perceived relief rather than objective recovery; the driver's alarm and surroundings remain unchanged.

Text from M: “well?”

| Player response | M replies |
| --- | --- |
| “I feel normal.” | “good. now get moving.” |
| “The ritual is complete.” | “then bring the carriage.” |
| “I stopped a car with a fireball.” | “knew the stick was worth keeping.” |

All options set the same objective: **Take the car.**

### L3.05 — Become the driver

1. Player walks around to the driver's door. The engine is still running, and the driver's hands remain on the wheel.
2. F opens the door. Driver: “No. You're not getting in.”
3. A second deliberate F interaction, **Take the driver's seat**, begins the takeover. Use a short staged struggle: the protagonist pulls the driver out, the view turns with the open door, and the driver lands off camera on the shoulder. No fighting system or graphic injury is needed.
4. During the staging, the vacuum is placed in the rear seat. A short visual confirms it traveled with the player. The wand is laid across the passenger seat, now visibly an ordinary branch.
5. Blend into the same camera position and dashboard composition as the prologue. Objective: **Drive to M.**

In the mirror, the displaced driver is left beside the bridge, with the path to the tent behind him. This repeats the opening composition of a person beside the bridge watching a car depart. It suggests the next turn of the cycle without adding a second explanatory scene.

### L3.06 — Departure and ending

Give the player approximately 30 seconds of actual forward driving. Use forgiving steering, the same road language as the opening, and the same radio track at the matching musical phrase. Keep the final lane clear; an accidental crash resets to departure, not to the store.

The phone vibrates on the passenger seat. An automatically visible message reads: “where are you?” No response is required while driving.

Pass another sign: **UNSPECIFIED CITY — A FRESH START**.

The objective changes from **Drive to M** to **Start your new life**. After the driving threshold and final road marker are both reached, the screen slowly fades to black while the car and radio continue.

Credits begin. No moralizing epilogue and no promise that the protagonist has actually escaped. The last sound is the same phone ringtone heard beneath the bridge.

## 8. State flow, checkpoints, and recovery

Use one authoritative story phase with explicit enter, update, and exit behavior. Persistent facts such as courtroom responses, inspected evidence, and carried story objects belong in saved story data. Do not use independent overlapping timers as the source of truth for story progression.

| Phase group | Required completion condition | Next group |
| --- | --- | --- |
| Prologue fueling | Nozzle returned, player seated, engine started | Intro driving |
| Intro driving | Driving requirement plus bridge marker | Scripted collision |
| Bridge arrival | Opening conversation completed | First sleep |
| First morning | Wand collected, store reached | First shopping trip |
| First shopping | Cart collected, vacuum loaded, threat performed | First exit |
| First exit | Loaded cart outside plus five seconds | Drone arrest |
| Courtroom entry | Defense table reached, opening stand/sit completed | Identity and charges |
| Identity and charges | Identity and allegation responses committed | Evidence and witness |
| Evidence and witness | Evidence screen inspected, clarification and interruption chosen | Defendant statement |
| Defendant statement | Statement branch and any burger follow-up completed | Suitability questions |
| Suitability questions | Both responses committed | Ruling and release |
| Ruling and release | Ruling heard, player exits to holding corridor | Release transition |
| Released | Notice dismissed and M conversation completed | Second shopping trip |
| Second shopping | Vacuum taken through exit | Return to bridge |
| Home | Vacuum delivered and M conversation completed | Second sleep |
| Final morning | M conversation completed, equipment staged | Road encounter |
| Road encounter | Player commits, casts, scripted stop completes | Ritual |
| Ritual | Equipment retrieved, abstract interaction completed, M answered | Car takeover |
| Takeover | Door opened and seat interaction confirmed | Final drive |
| Final drive | Thirty seconds of forward driving plus final marker | Credits |

Recommended saves: bridge arrival, first morning, courthouse arrival, first courtroom question, each completed courtroom exchange, release outside store, successful second theft, final morning, roadside preparation, and final departure. A checkpoint restores a coherent scene: object locations, NPC states, cart contents, lighting, dialogue position, and objective. Within court, save the current question and committed choices so reloads neither erase prior statements nor replay the whole hearing.

Recovery rules:

- Required dropped items remain marked. If they fall into water or outside playable bounds, restore them at the latest safe position.
- The vacuum box is attached to the cart while shopping. It cannot silently fall out.
- If the cart becomes stuck, offer a nearby **Reset cart** interaction that restores the cart and its contents to the last clear aisle or path marker.
- Wand recovery remains available from the cart during store scenes and from the tent after transfers. Never duplicate it across checkpoint loads.
- Store staff cannot be killed, and the drone cannot be defeated in this story slice. Experimental casting produces presentation and a short reaction, not a broken quest.
- Traffic runs in staged encounters rather than full simulation. Cars do not arrive during phone conversations or run the player over while waiting for input.
- The final target car is locked against incidental damage or physics displacement during its story sequence.
- On long inactivity, repeat the current objective or a subtle directional cue. Never auto-select a dialogue response.
- Dialogue and evidence-inspection selections consume their inputs so confirming an answer cannot also cast, drop an item, or enter a vehicle.
- Courtroom choices have no timer. Silence does not automatically become a sensible answer, a failure, or an extra monologue. The proceeding waits for selection.
- Every courtroom branch gets an immediate distinct reaction and a saved transcript entry. All branches converge on release; none secretly produces a competent defense or a different sentence.
- Hearing resumes at exchange boundaries. Skip controls apply the same statement, response flags, and outcome as watching the full performance.
- Restarting during a cutscene restores its preceding checkpoint with clean event flags. One-shot dialogue, taser events, and fades cannot run twice in one entry.
- Allow cutscene skipping after first viewing. Skipping applies the same final state as watching the complete scene.

## 9. Build order

The existing project has held-item handling in `game/item.*`, registered spells and level logic in `game/game.*`, spell effects in `game/attack_wizard.*`, and car code in `physics/car.*`. Treat these as starting points to inspect, not proof that every proposed feature already works. Keep this story's configuration separate from the current spell sandbox so unused sword and darkblast bindings do not bypass scripted encounters.

### Step 1 — Lock the route and sequence

Build graybox versions of the bridge, tent path, store, approach road, holding corridor, and courtroom. Place named markers for every scripted arrival, interaction, spawn, and exit.

**Done when:** someone can walk the complete physical route, push the cart through it, and drive both road segments without catching on geometry.

### Step 2 — Build the story foundation

Implement the authoritative story phase, objective display, interaction priority, input modes, fade transitions, and checkpoint serialization. Add a developer phase selector and an on-screen current-phase display.

**Done when:** each phase can be entered directly with correct objects, lighting, and controls, and can be restored after restarting the game.

### Step 3 — Make the full ending reachable

Connect the entire story using placeholder boxes, text, and timed camera blends. Implement the nozzle, tent, cart loading, courtroom seating and standing, evidence inspection, equipment placement, and car entry as simple contextual actions. Use placeholder subtitles for all speaking roles.

**Done when:** a new game can reach credits without developer commands, even with rough visuals. This is the first complete story build.

### Step 4 — Implement conversations and the courtroom

Add M's messages, selectable replies, persistent conversation position, the seven courtroom beats, evidence inspection, the turkey-burger monologue and follow-ups, and wizard/burger callbacks. Store dialogue as content records with stable identifiers and event conditions. Track the chosen statement, inspected exhibits, current exchange, and court-wizard claim independently.

**Done when:** every listed response leads to the correct shared outcome, interrupted conversations resume, every terrible choice gets its own reaction, and phone/courtroom interfaces never leak input into gameplay.

### Step 5 — Finish the store and legal sequence

Add cashier reactions, cart staging, the five-second exterior timer, drone approach, arrest, courtroom transfer, ruling, release, and the second theft. Ensure the second exit never schedules another arrest.

**Done when:** both visits play differently for the intended reasons, and the player can return home with the vacuum after release.

### Step 6 — Finish the opening and final encounter together

Build paired camera compositions for road entry, bridge-side awakening, departing car, mirror view, and dashboard. Add the staged fireball without destructive car damage. Complete the abstract ritual and car takeover.

**Done when:** the role reversal is understandable from the images, the car remains usable, and both drives reach their transitions reliably.

### Step 7 — Add art, sound, and subjective effects

Replace placeholders. Add the distinctive coat, stick, phone, vacuum branding, drone, courtroom cast, case packet, evidence screen, tent, store signage, original/licensed radio music, ambient sound, and voice performances. Keep drone delivery polite, certain, and emotionally flat; M is casual; the cashier is tired and concerned. Courtroom actors mostly deliver straight lines and react to the protagonist rather than speaking in nonstop punchlines.

**Done when:** sound and visual changes support the character's interpretation, all text is readable, and the reduced-effect settings preserve the story cues.

### Step 8 — Playtest for comprehension and completion

Run a fresh first-play session without explaining the premise. Observe where players get lost, misunderstand a required interaction, miss the role reversal, or wait for a choice to create an unsupported branch.

**Done when:** players can reach credits without help, understand that the final driver role echoes the opening, recognize the fictional system's contradiction, and distinguish the protagonist's magical interpretation from the ordinary reactions around him.

## 10. Acceptance pass

- Complete one uninterrupted new-game-to-credits run.
- Exercise every courtroom response, case-packet unlock, and turkey-burger follow-up through targeted phase loads; confirm distinct reactions and convergence on the same release.
- Confirm both the court-wizard and turkey-burger callbacks work, and neither appears without its triggering choice.
- Reload and skip during the long monologue; confirm no duplicate lines, lost selections, or incorrect release state.
- Verify the court scene never accepts a formal guilty plea or announces an acquittal, while the later release notice still delivers its intended contradiction.
- Save and reload every checkpoint, including with the phone open or a story item carried where supported.
- Pause during both drives and the five-second store timer; confirm paused time does not advance them.
- Walk backward through the store, abandon and recover the cart, drop the wand, leave the phone unread, and linger after the second exit. None should block or repeat the story incorrectly.
- Confirm that two spell keys used as dialogue selections cannot fire sandbox spells.
- Verify the first threat and final takeover require deliberate player input.
- Confirm skips, restarts, and reduced-motion settings produce the same final story state.
- Check the final fireball never destroys the target car or removes its driver before the takeover.
- Ensure subtitles cover dialogue continuing over black and release text stays until dismissed.
- Verify the ending requires forward travel, works at different frame rates, and cannot trigger while parked.
- Remove developer phase controls from the shipping interface while preserving them for development builds.

## 11. Scope for the first finished version

Ship one complete linear story, one store, one drone, one cashier, one final driver, a compact courtroom with judge, attorney, prosecutor and clerk/bailiff, one functional spell presentation, and the reusable bridge map. Reuse the cashier on the witness monitor. Keep phone and courtroom choices expressive but convergent. Defer open-world traffic, general NPC combat, branching legal outcomes, additional spells, multiplayer, and alternate endings until this complete loop is playable and understandable. The two-hour target remains an expansion goal: validate the actual length of each playable section rather than padding this hearing to make up the difference.
