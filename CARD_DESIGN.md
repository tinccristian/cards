# Medical Deckbuilder — Card Design Document

## Overview

All planned cards, mechanics, and systems. Cards ordered by rarity, then mana cost within rarity.
Existing 10 cards not listed here. New total: 55 new cards → 65 in pool.

---

## OUTER GAME LOOP — The Lab

### Concept
A 2D isometric medicine lab. This is the persistent meta-layer that wraps the existing card combat loop.
Each "run" is a single surgical operation performed on a patient. The card game is the surgery itself.

---

### Flow

```
Patient arrives at lab
    ↓
Patient describes disease / symptoms
(determines map layout, enemy types, difficulty)
    ↓
Patient gets on the operating table
    ↓
Pre-operation phase (anesthesia, prep tools)
(player applies pre-op effects that carry into the operation)
    ↓
OPERATION BEGINS → existing map + node + combat loop
    ↓
Nodes completed = surgery complete
    ↓
Post-operation: calculate patient satisfaction
    ↓
Gold reward → spend on cards + lab improvements
    ↓
Next patient
```

---

### The Lab (Isometric View)

The main screen between operations is the isometric lab. Visible elements:

- **Operating table** — centre of the room. Patient lies here during a run.
- **Equipment stations** — unlockable lab improvements placed around the room (see Lab Upgrades below).
- **Reception area** — where the next patient appears and waits.
- **Card cabinet** — where you manage your deck and buy new cards.
- **Notice board** — shows current patient queue, upcoming difficulty, optional side jobs.

The lab is not a mini-game — it is a presentation layer. Clicking things opens menus. The isometric view is mood and progression tracking, not navigation.

---

### Patient Intake

Each patient has:
- **A name and brief description** — "Maria, 54. Fungal lung infection, progressive."
- **A disease type** — determines which enemies appear in the map. (Fungal → Fungus enemies. Viral → future Virus enemies. Bacterial → future Bacteria enemies.)
- **A difficulty tier** — Standard, Complicated, Critical. Higher = better gold + better card rewards.
- **A patience threshold** — how many turns the operation can take before quality starts degrading. Longer operations on low-patience patients lose satisfaction faster.

Patients are procedurally generated — random name, disease, tier — with the disease pool expanding as more enemy types are added.

---

### Pre-Operation Phase

Before the map launches, the player performs prep steps on the patient.
Each prep tool is selected from a small set of available options:

| Tool | Effect carried into operation |
|---|---|
| **Anesthesia** | Patient starts with no pain. Playing cards that would normally lower quality don't during the first 2 turns. |
| **Scalpel prep** | First combat starts with enemy Hemorrhage 2 already applied. |
| **Oxygen mask** | Player starts with Regeneration 2 for 3 turns. |
| **Antiseptic wash** | Enemy starts with Brittle applied. First hit deals +50% damage. |
| **Blood pressure cuff** | Player starts with 1 extra mana on turn 1. |
| **Sedation drip** | Enemy's first turn is skipped. |

The player picks **2 of the available 4** (random selection of available tools). This is a quick meaningful choice before the operation starts — not randomised outcome, just which two advantages you take.

As the lab improves, more tools become available in the rotation.

---

### Operation Quality

A quality meter (0–100%, starts at 100%) tracks how well the operation is going.
It is always visible during combat — shown as a patient status panel on the UI (separate from the player's own HP).

**Quality decreases when:**
- The player takes damage (each hit lowers quality proportionally to damage dealt)
- A combat takes more turns than the patient's patience threshold
- The player ends a turn with no block and takes a large hit
- Special "complication" events trigger (future: random mid-map events)

**Quality increases when:**
- The player heals
- The player cleanse debuffs (clean treatment)
- The player defeats an enemy quickly (decisive surgery)
- Specific lab upgrades are installed

**Quality does NOT drop below 20%** — the patient always survives unless a separate instant-death condition triggers (not currently designed).

---

### Post-Operation Satisfaction & Gold

After all nodes are cleared, the operation ends.
Gold earned = base reward × satisfaction multiplier.

| Final Quality | Satisfaction | Multiplier |
|---|---|---|
| 90–100% | Exceptional | 2.0× |
| 70–89% | Good | 1.5× |
| 50–69% | Adequate | 1.0× |
| 30–49% | Poor | 0.7× |
| 20–29% | Malpractice | 0.4× |

The post-operation screen shows:
- Patient name and disease
- Final quality with a simple animation (line graph of quality over time)
- Gold earned
- Any bonus rewards (card drops, lab parts) from performing well

---

### Lab Upgrades

Spent gold on permanent improvements to the lab. Upgrades persist across all patients (runs).
Each upgrade has a visible physical representation in the isometric view.

**Examples:**

| Upgrade | Cost | Effect |
|---|---|---|
| **Candy Bowl** | 40g | Patients start 5% more satisfied. Cheap morale. |
| **Blood Bank** | 80g | Player starts each operation with +10 max HP. |
| **Sterilisation Unit** | 100g | Pre-op Antiseptic Wash is always available (never excluded from the 4 options). |
| **Cardiac Monitor** | 120g | Operation quality is shown to 1 decimal point. Quality drop events display the exact amount lost. |
| **Advanced Anesthesia Drip** | 150g | Anesthesia effect lasts 4 turns instead of 2. |
| **Bone Saw** | 130g | Scalpel Prep now applies Hemorrhage 4 instead of 2. |
| **Lab Computer** | 200g | Shows the patient's full map (all nodes + enemy types) before accepting the case. |
| **Resident Surgeon** | 250g | Once per operation, auto-play a random defense card for free at turn start (NPC assistant). |
| **Organ Freezer** | 300g | Organ cards you play persist into the next operation (one organ carries over between runs). |
| **VIP Waiting Room** | 180g | Unlocks Critical tier patients. Higher difficulty, much higher gold. |

Lab upgrades are the meta-progression. A new lab starts bare — one table, no frills.
A developed lab makes operations safer, more profitable, and more strategically interesting.

---

### Thematic Notes

- The card game is the surgery. You are the immune system — or the interventions — fighting on behalf of the patient.
- The patient's wellbeing (quality meter) is separate from the player's HP. You can survive at 10 HP but if the patient quality dropped too far, the surgery was a failure.
- The isometric lab view should feel like a real medical environment: clean, clinical, slightly cold. Progress shows in the room — a better lab looks different from a starter lab.
- Sound design: quiet clinical ambience in the lab (monitors, AC, distant footsteps). The card combat keeps its existing audio. The transition between lab and combat is a deliberate screen wipe.

---

## New Card Types

### Organ Cards
A fourth card type alongside Attack, Defense, and Skill.
Organs are played once and remain permanently active for the rest of combat.
They do not go to the discard pile — they enter a persistent "Body" zone.

**UI Placement:**
- A horizontal "Body" strip appears above the player's hand the moment the first organ is active.
- Each organ shows as a small card thumbnail in that strip (max 4 visible slots).
- Organs glow faintly at all times to indicate they are active.
- When an organ triggers (e.g. Pacemaker healing at turn start), it briefly flashes brighter.
- If a 5th organ is played, it replaces the oldest one — or hard-cap at 4 (design decision).
- Organ thumbnails show a small animated icon for their effect (a pulse line, a bone, a bypass wire, etc.)

**Acquisition:**
Organs appear less often in standard combat rewards. They are more common in the shop,
guaranteed from boss fights, and available in the Gambling Den event at high buy-ins.

---

## New Status Effects

### Hemorrhage *(enemy debuff)*
The enemy takes X damage at the start of *their* turn. Does not expire on its own — stacks increase it.
> **Icon:** a dark red drip icon below enemy health bar, with a stack counter.
> **UI:** the stack counter increments visibly when new Hemorrhage is applied.
> **Feel:** A slow bleed. You set it up, then play defensively and watch it tick them down every single turn. Long fights punish the enemy. Short fights you won't feel it.

### Sepsis *(enemy debuff)*
All Poison damage the enemy takes is doubled. Single application — does not stack.
> **Icon:** a biohazard symbol, dark green, glows briefly when poison ticks.
> **Feel:** A multiplier, not damage. Worthless without Poison already applied. Combined with Poison 3: enemy takes 6/turn. The ultimate poison combo piece.

### Brittle *(enemy debuff, single use)*
The next attack that hits the enemy deals +50% bonus damage. Consumed on hit.
> **Icon:** a cracked bone, single-use indicator (greyed after consumed).
> **Feel:** A setup piece for a nuke. Play Vaccine, then follow with Radiation Blast for 30 damage instead of 20. Timing matters.

### Regeneration *(player buff)*
Heal X at the start of each of your turns. Lasts a set number of turns (Regen 3 for 2 turns = heal 3 for 2 turns).
> **Icon:** a green cross with a small pulse wave radiating from it.
> **Feel:** Sustained healing without spending a card each turn. Changes the math on tanking hits deliberately.

### Calcification *(player buff)*
Your block does not expire at the end of your turn. Lasts X turns.
> **Icon:** a white shield with a lock icon overlay.
> **Feel:** Block becomes savings. Stack defensively for two turns, then unleash without worrying about expiry. Big in slow, deliberate decks.

### Inflammation *(player debuff, enemy-inflicted)*
All cards cost 1 more mana to play. Lasts X turns.
> **Icon:** a red flame on the mana crystal, stack count.
> **Feel:** The most disruptive debuff in the game. Ruins turn economy. Priority cleanse target. A future enemy that inflicts this will be immediately threatening.

### Surge *(player buff, single use)*
Your next attack deals double damage. Consumed when the next attack is played.
> **Icon:** a glowing orange arrow on the first attack card in hand.
> **UI:** the badge is visible on the card face while Surge is active.
> **Feel:** A hidden multiplier waiting to go off. You hold your best attack for one more turn and then it hits for 2x. Pairs with Adrenaline Gland organ for 4x first attack.

---

## Gambling System

### Jackpot (5% proc chance on all [Gambling] cards)
Any card tagged **[Gambling]** has a hidden 5% chance to trigger JACKPOT when played.
All numeric effects on that card triple. A distinct gold screen flash and audio sting fire.
> **Feel:** Rare enough that it shocks you. Frequent enough that you always think "maybe this time." The entire gambling sub-deck is more exciting because of this one rule.

### The Streak
The game tracks your current win/loss record for gambling coin flips this run.
After **3 consecutive wins**: gambling cards gain +1 to their best outcome this combat.
After **3 consecutive losses**: the next gambling card's worst outcome is replaced with a neutral result.
> **Feel:** The house gives you a cushion. You're never fully mercy of pure RNG. Rewards leaning in.

### Blood Pressure *(optional future system)*
A hidden counter (0–10) that rises by 1 each time you take damage, resets each combat.
Some cards (tagged [BP]) scale their effect with the current Blood Pressure value.
Higher BP = more explosive effects. Rewards deliberately tanking hits early.

---

## How to Obtain Cards

| Source | Details |
|---|---|
| **Combat Rewards** | After each fight: choose 1 of 3 cards. Rarity weighted by map depth. Common early, rare late. |
| **Shop** | Buy specific cards for gold. Remove a card from deck for a fee. Refresh shop for smaller fee. |
| **Boss Loot** | Guaranteed 1 Rare + 1 Organ after each boss fight. No choice — both are given. |
| **Noah Event** | Existing event. Trade, transform, gain, or remove cards. |
| **Gambling Den** *(new event)* | Wager gold. Low bet: 3 common cards (pick 1). High bet: 3 rare cards (pick 1). All-in: random Organ. Lose the wager and gain nothing. Jackpot chance applies. |
| **Operating Theater** *(new event)* | Option A: Remove 1 card, gain 1 uncommon. Option B: Risk Surgery — 50%: gain 2 uncommons. 50%: lose 2 random cards from deck. |
| **Field Kit** *(rare map pickup between nodes)* | Grants 2 random common cards immediately. No choice. |

---

---

# ORGAN CARDS
*Played once. Stays in Body zone for the rest of combat. Does not go to discard.*
*Ordered by mana cost.*

---

### Pacemaker | 1 mana | Organ *(IMPLEMENTED)*
**Effect:** At the start of each of your turns, heal 2 HP.
**Art:** A pixelart pacemaker device — clean chrome casing, a rhythmic pulse line extending from it into darkness.
**UI/Feel:** Pulses with a brief heartbeat shimmer in the Organs strip at turn start. The quiet constant. Adds up over a long fight. Makes you willing to take calculated hits early because you know you're recovering.

---

### Blood Filter | 1 mana | Organ
**Effect:** At the end of each of your turns, cleanse 1 debuff from yourself.
**Art:** A dialysis machine — tubes carrying dark blood in, clean red blood out. Gauges and beeping monitors.
**UI/Feel:** Lights up at end of turn when it triggers. Slow counter to poison stacking enemies. Doesn't save you immediately — it stops the spiral. Against Inflammation (cards cost more): this removes it in 1 turn.

---

### Reinforced Bones | 2 mana | Organ
**Effect:** All block you gain is increased by 3.
**Art:** An X-ray of a forearm — bones thick and bright white, heavy calcium deposits gleaming.
**UI:** Block numbers in-game display the base amount with a "+3" tint added visually. Bandage tooltip reads "4 (+3)" when this organ is active.
**Feel:** Every defensive card quietly becomes better. Bandage goes from 4 to 7. Cough Syrup goes to 11. No dramatic moments — just cumulative power.

---

### Adrenaline Gland | 2 mana | Organ
**Effect:** The first attack card you play each turn deals double damage.
**Art:** An adrenal gland perched atop a kidney, crackling with orange energy and heat haze above it.
**UI:** A faint "2×" badge appears on the first attack in your hand at turn start. Badge disappears when that card is played.
**Feel:** Forces you to lead with your best attack every single turn. Changes sequencing completely — you stop playing attacks freely and always open with your highest damage card.

---

### Neural Bypass | 2 mana | Organ
**Effect:** The first card you play each turn costs 0 mana.
**Art:** A stylized neuron — a gold wire runs directly across the synapse gap, bypassing it entirely.
**UI:** The leftmost card in hand gets a small gold "0" badge at turn start. It disappears when that card is played.
**Feel:** With 3 base mana you effectively get 4 plays per turn. Changes deckbuilding — cheap cards that felt redundant now make the cut because the first one is free.

---

### Cortisol Regulator | 2 mana | Organ
**Effect:** Reduce all incoming damage by 2 (minimum 1 damage always gets through).
**Art:** A small implanted capsule beneath pixelart skin, glowing faint orange. Label reads "CORT-RX."
**UI:** Incoming damage numbers show a brief "-2" reduction text in muted blue before displaying the final damage.
**Feel:** Invisible armour. Against swarms of small hits (3, 4, 5 damage attacks), extraordinary. Against one massive nuke: barely noticeable. Matchup-dependent organ.

---

### Synthetic Lung | 3 mana | Organ *(IMPLEMENTED)*
**Effect:** Whenever you draw one or more cards (from any source), draw 1 additional card.
**Art:** A mechanical lung — half organic tissue, half chrome scaffolding — expanding and contracting. Steam at the joints.
**UI:** The bonus drawn card has a brief chrome shimmer border as it enters the hand.
**Feel:** Snowballs hard. Ambulance (draw 3) now draws 4. Bone Marrow Surge draws 5. Your hand gets crowded fast — requires a tight deck or discard synergies to use well.

---

### Adaptive Immunity | 3 mana | Organ
**Effect:** Each time you are Poisoned this combat, permanently gain 1 Poison Resistance (stacks; reduces poison damage taken by 1 each, minimum 0).
**Art:** A veteran white blood cell — battle-scarred, medals hanging from it, standing tall.
**UI:** Small "ADAPT: X" stack counter shown below the organ thumbnail in the Body strip.
**Feel:** Gets good specifically against poison-heavy enemies. Meaningless against non-poison enemies. The first few stacks hurt. By stack 4 you take 0 poison damage. High-variance organ depending on what you're fighting.

---

### Histamine | 2 mana | Organ *(IMPLEMENTED)*
**Effect:** Each time you draw a card, deal 1 damage.
**Art:** A mast cell packed with granules — round, studded with reactive packets ready to burst outward. Clinical cross-section view.
**UI/Feel:** Quietly deadly in draw-heavy turns. Ambulance becomes draw 3 + deal 3. Bone Marrow Surge becomes draw 4 + deal 4. Synergizes with Synthetic Lung (each bonus draw also triggers). Rewards building around card draw.

---

---

# COMMON CARDS
*Ordered by mana cost.*

---

### Coin Toss | 0 mana | Gambling
**Effect:** Flip a coin. Heads: gain 2 mana. Tails: draw 2 cards. No losing outcome — just different flavours of good.
**Art:** A coin mid-flip — one face a glowing red mana crystal, the other face a card back.
**Feel:** Safe entry into gambling. Teaches the coin-flip rhythm without fear. Good in any deck.

---

### Lancet | 0 mana | Attack
**Effect:** Deal 3 damage. Draw 1 card.
**Art:** A tiny lancet blade with a single dark droplet hanging from the tip. Minimal and precise.
**Feel:** Replaces itself. Ping damage with card flow. Good in aggro decks that want to stay in motion.

---

### Placebo | 0 mana | Gambling *(IMPLEMENTED)*
**Effect:** 50% chance: draw 3 cards. 50% chance: nothing happens.
**Art:** A single gleaming white sugar pill. A question mark floating softly above it.
**Feel:** Pure variance. Sometimes carries a turn. Sometimes wastes a play. Addicting to include because the upside is so large for 0 mana.

---

### Pulse Check | 0 mana | Skill
**Effect:** Look at the top 3 cards of your deck. Put them back in any order.
**Art:** A finger on a wrist. An EKG heartbeat line floating above, reading the rhythm.
**Feel:** No direct power. Pure information and sequencing. Enables next-turn combos by stacking what you need.

---

### Adrenaline Shot | 1 mana | Skill
**Effect:** Draw 2 cards.
**Art:** An epipen pressed against skin, plunger fully depressed, rings of impact expanding outward.

---

### Bandage | 1 mana | Defense
**Effect:** Gain 4 block.
**Art:** A bandage roll mid-wrap. Clean white against dark clinical background.

---

### Blood Clot | 1 mana | Skill
**Effect:** The enemy cannot gain block until the start of your next turn.
**Art:** A dark red clot plugging a cross-section of blood vessel entirely. Clinical and ominous.
**Feel:** Situational but powerful. Enemy about to shield? Deny it, then hit. Useless against non-blocking enemies — dead card against some fights.

---

### Deep Breath | 1 mana | Defense
**Effect:** Gain 3 block. Draw 1 card.
**Art:** A pair of lungs expanding fully, ribcage silhouette visible, light filtering through tissue.

---

### Fever Response | 1 mana | Defense
**Effect:** Gain 6 block.
**Art:** A thermometer — mercury column spiking into the red, glass cracking at the top.

---

### Ibuprofen | 1 mana | Attack
**Effect:** Deal 5 damage.
**Art:** Two orange pills mid-air above an open blister pack. Clean clinical lighting.

---

### Infection Control | 1 mana | Attack
**Effect:** Deal 8 damage. Can only be played if the enemy has at least 1 debuff.
**Art:** A white blood cell stamping a red QUARANTINE seal onto a cowering virus.
**Feel:** Conditional punch. 8 damage for 1 mana is elite when triggered. Forces setup. Core card in debuff-focused decks.

---

### Saline Flush | 1 mana | Defense
**Effect:** Gain 4 block. Heal 2 HP.
**Art:** A clear saline IV bag, fluid level visible, a slow droplet hanging from the tube.

---

### Swab | 1 mana | Attack *(IMPLEMENTED)*
**Effect:** Deal damage equal to the number of cards in your hand × 2.
**Art:** A cotton swab, fully saturated and dark. Close-up. Macro medical photography feel.
**Feel:** Best at the start of your turn when your hand is full. Terrible as a turn-ender. Rewards playing it first.

---

### Triage | 1 mana | Skill
**Effect:** Gain 2 mana next turn. Draw 1 card.
**Art:** Emergency colour-coded triage tags — red, yellow, green — hanging from wrists in a row.

---

### White Cell | 1 mana | Attack *(IMPLEMENTED)*
**Effect:** Deal 3 damage. If the enemy is Poisoned, deal 6 damage instead.
**Art:** A white blood cell pursuing a glowing virus through a vessel. Predator energy.
**Feel:** Always fine. Excellent when Poison is active. A good reason to run Syringe — the synergy is cheap and obvious.

---

---

# UNCOMMON CARDS
*Ordered by mana cost.*

---

### Fever Response | 1 mana | Skill *(IMPLEMENTED)*
**Effect:** This turn, each card you draw deals 1 damage.
**Art:** A thermometer in the red — heat shimmer rising from it, cells in the background visibly accelerated and agitated.
**Feel:** Play it early in a draw-heavy turn. Ambulance after this = draw 3, deal 3. Pairs perfectly with Histamine — organ gives permanent draw-damage; this gives a spike turn of double damage per draw.

---

### Anticoagulant | 1 mana | Skill
**Effect:** Apply Hemorrhage 4 to the enemy.
**Art:** A vial of anticoagulant — dark red contents, a medical blood-drop label.
**Feel:** Zero direct damage. Pure setup. A Hemorrhage engine card. Play it on turn 1 and it pays off every subsequent turn. Core card in bleed builds.

---

### Surgical Strike | 1 mana | Attack
**Effect:** Deal 12 damage. Requires the enemy to have 2 or more debuffs to play.
**Art:** A scalpel moving with laser precision — a single clean bright incision line left behind.
**Feel:** 12 damage for 1 mana is top-tier. The condition forces a debuff build. Anticoagulant + Syringe into this = 12 damage on turn 2 for 3 mana total.

---

### Anti-inflammatory | 2 mana | Attack
**Effect:** Deal 8 damage. Cleanse 1 debuff from yourself.
**Art:** An inflamed red cell being doused with cool blue medicine. Visible relief.

---

### Blood Pressure Spike | 2 mana | Gambling
**Effect:** Roll 1d6. Deal that number × 4 damage. (Minimum 4. Maximum 24.)
**Art:** A blood pressure cuff inflating dangerously — the needle deep in the red zone, numbers flickering.
**Feel:** Average outcome is 14 damage for 2 mana. Sometimes 4 (rough). Sometimes 24 (incredible). Never routine. Goes in gambling decks for the ceiling.

---

### Bone Marrow Tap | 2 mana | Skill
**Effect:** Draw 3 cards.
**Art:** A biopsy needle entering bone — golden marrow visible inside the cavity.

---

### Clinical Trial | 2 mana | Gambling
**Effect:** Flip a coin. Heads: deal 14 damage and draw 2 cards. Tails: take 8 damage.
**Art:** A scientist at a whiteboard — half the board shows success data, the other half failure data. Both futures visible simultaneously.
**Feel:** High stakes. Tails genuinely hurts. Heads often wins fights. Key card in gambling-focused decks. With The Streak system, losses bring you closer to a guaranteed win.

---

### Cytokine Storm | 2 mana | Attack
**Effect:** Deal 6 damage. Apply Hemorrhage 3 to the enemy.
**Art:** A chaotic swirl of biological particles — red and white cells colliding, frantic biological storm.
**Feel:** Does both jobs at once — not the best at either, but reliable as an opener. The Hemorrhage compounds every subsequent turn.

---

### Double or Nothing | 2 mana | Gambling
**Effect:** Deal 10 damage. If the enemy survives this hit, they gain 8 block.
**Art:** A pair of dice — one showing a high number, one showing a low one. The outcome uncertain.
**Feel:** Great for finishing or big chunk damage. Bad against high-HP enemies — you're literally arming them if they survive. Requires reading the enemy's health.

---

### Epinephrine | 2 mana | Skill
**Effect:** Your next card costs 0 mana. Draw 1 card.
**Art:** A glowing autoinjector pen — stylized pulse wave radiating behind it.
**Feel:** The enabler. Combine with Surgery for 3-mana full heal setup. Combine with Chemotherapy for free nuclear damage. Flexible in any deck.

---

### Fibrin Mesh | 2 mana | Defense
**Effect:** Gain 8 block. Apply Regeneration 3 for 2 turns to yourself.
**Art:** A lattice of fibrin proteins weaving together over a wound — biological scaffolding forming in real time.
**Feel:** Solid block plus slow healing. Not exciting but deeply practical in long fights. The regeneration adds up to 6 healing over 2 turns.

---

### IV Drip | 2 mana | Skill *(IMPLEMENTED)*
**Effect:** Heal 8 HP.
**Art:** A clear IV bag hanging — fluid level dropping. Warm clinical amber glow.

---

### Morphine | 2 mana | Skill
**Effect:** Heal 6 HP. Draw 1 card.
**Art:** A morphine drip bag — soft amber glow, edges blurred with warmth and stillness.

---

### Risk Assessment | 2 mana | Gambling
**Effect:** Before playing, declare your choice: deal 10 damage, gain 10 block, OR heal 8. The card does exactly what you declared.
**Art:** Three unmarked doors, each with a different medical symbol. A doctor pointing deliberately at one.
**Feel:** No randomness at all — it's a decision card. The "gambling" is that you commit before you see what's coming. Surprisingly tense. Flexible enough to fit in any deck.

---

### Vaccine | 2 mana | Defense
**Effect:** Gain 6 block. Apply Brittle to the enemy (next attack against them deals +50% damage).
**Art:** A vaccine vial and needle side by side — faint molecular structure pattern in the background.

---

### Cortisol Surge | 3 mana | Defense
**Effect:** Gain 14 block. Gain 2 mana.
**Art:** An adrenal gland firing — a visible release of chemical signal rippling outward.
**Feel:** Expensive but pays for itself partially. 14 block is among the highest raw block values. The 2 mana refund means it costs 1 net mana in effect.

---

### Defibrillator | 3 mana | Attack
**Effect:** Deal 15 damage.
**Art:** Two shock paddles arcing electricity between them, charged and ready to discharge.

---

### Septic Compound | 3 mana | Skill
**Effect:** Apply Sepsis to the enemy. Apply Poison 2 to the enemy.
**Art:** A dark biological compound dripping through a filter into a flask — viscous and threatening.
**Feel:** Sepsis doubles all poison damage. This card immediately puts them at 4 poison/turn (2 × 2). Every subsequent Syringe or Chemotherapy will deal double poison damage too. Set it early.

---

### White Cell Wall | 3 mana | Defense
**Effect:** Gain 16 block.
**Art:** A phalanx of white blood cells shoulder-to-shoulder, translucent shields raised. Biomilitary.

---

---

# RARE CARDS
*All rare cards include an audiovisual effect when played.*
*Ordered by mana cost.*

---

### Second Opinion | 1 mana | Gambling | RARE *(IMPLEMENTED)*
**Effect:** Reveal the top 5 cards of your deck. Play 1 of them for free. Shuffle the rest back.
**Art:** Two doctors in white coats pointing at a diagnostic chart — animated disagreement, then consensus. The chart shows 5 possible outcomes.
**AV Effect:** A quick scan-line sweep across the screen left-to-right (like a diagnostic read), then a soft ping sound as the chosen card glows. Sound: UI scan hum → clean selection chime.
**Feel:** Controlled gambling — you see the options, you choose. Intelligence over luck. Lets you pull the card you need right now from the top of your deck.

---

### Blind Faith | 1 mana | Gambling | RARE
**Effect:** Roll 1–3 randomly. [1] Take 5 damage. [2] Gain 5 block. [3] Heal 5 HP and draw 2 cards.
**Art:** A blindfolded doctor administering a treatment by touch alone. Hands confident despite no sight.
**AV Effect:** Result [3] only: gentle green shimmer blooms outward from screen center, holds briefly, fades. Sound: soft chime.
**Feel:** Each result is meaningfully different. The fear isn't catastrophic loss — it's not knowing which flavour you land on. With The Streak system, three losses guarantee the best result next.

---

### Chemotherapy | 2 mana | Attack | RARE *(IMPLEMENTED)*
**Effect:** Deal 15 damage. Apply Poison 2 to yourself.
**Art:** An IV bag with a skull-and-crossbones label, fluid dripping into a vein. Beautiful and terrible.
**AV Effect:** Screen flashes sickly yellow-green — a slow nausea wave rolls from the top of the screen to the bottom and fades over 2 seconds. The colour briefly desaturates then returns. Sound: a chemical drip sound → a queasy, settling low hum.
**Feel:** You are poisoning yourself to end the fight faster. Requires calculating whether you can outlast your own hand. Pairs with Blood Filter organ to negate the self-poison entirely.

---

### Overdose | 2 mana | Gambling | RARE
**Effect:** Gain 4 mana. Take 8 damage. Draw 3 cards.
**Art:** Pills spilling from an open prescription bottle — chaotic, wrong, exciting. Background blurred and spinning.
**AV Effect:** Screen flashes pure white, then over-saturates into vivid colour for 1 second (hyper-real, slightly bloomed) before fading back to normal. Sound: rush of wind → one strong heartbeat → silence.
**Feel:** Explosive. 4 mana + 3 cards means you can chain most or all of your hand this turn. The 8 damage is real. Do not play this at low HP. A gambling deck's centrepiece.

---

### All In | 3 mana | Gambling | RARE
**Effect:** Discard your entire hand. Deal 5 damage for each card discarded.
**Art:** A poker hand slammed face-down on a table. Cards flying outward. All chips pushed in.
**AV Effect:** Fast horizontal scan flash per card discarded (rapid-fire, one flash per card). Sound: rapid card-slap sounds building to a final heavy impact crash.
**Feel:** With 4 cards: 20 damage for 3 mana. With 6: 30 damage. High ceiling. Requires a full hand. Bad if you have 1 card. Devastating if you held back.

---

### Bone Marrow Surge | 3 mana | Skill | RARE
**Effect:** Draw 4 cards. Gain 1 mana.
**Art:** A cross-section of bone — the marrow cavity glowing molten gold, heat haze rippling above it.
**AV Effect:** Three concentric rings of warm gold light pulse outward from screen center, one after another. Sound: a deep resonant hum, like large bones vibrating at frequency.
**Feel:** The premier draw card. 4 cards + 1 mana refund. With Synthetic Lung organ: draws 5. With Neural Bypass: the first card drawn costs 0.

---

### Cascade Failure | 3 mana | Attack | RARE
**Effect:** Deal 4 damage for each card in your discard pile.
**Art:** A domino chain of cells falling — each one toppling the next in a clinical cascade. Entropy made visible.
**AV Effect:** Multiple rapid horizontal flashes, the spacing between them getting progressively shorter (building momentum). Sound: rapid ticks growing louder, then a single final crash.
**Feel:** Terrible on turn 1. Devastating on turn 5 after you have played most of your hand. A late-turn payoff card. 8+ cards in discard = 32+ damage.

---

### Immune Overdrive | 3 mana | Skill | RARE
**Effect:** Until the end of this turn, all cards cost 0 mana. At end of turn, discard your hand.
**Art:** A white blood cell glowing so intensely it is moments from bursting. Brilliance before sacrifice.
**AV Effect:** White-gold radiance fills the screen and pulses slowly for the entire duration of the turn (constant, subtle). Cuts to a brief darkness at turn end. Sound: a rising tone that holds during the turn → cuts suddenly when turn ends.
**Feel:** Play everything for free, lose what is left. Perfect planning = enormous turn. Sloppy planning = wasted hand. The highest ceiling of any Skill card.

---

### Septic Shock | 3 mana | Skill | RARE
**Effect:** Apply Sepsis to the enemy. Apply Hemorrhage 8 to the enemy.
**Art:** A body in crisis — veins darkening, cells collapsing, the immune system overwhelmed.
**AV Effect:** A dark organic green-black wash creeps in from the screen edges inward, holds for 0.5s, then a visceral flash back to normal. Sound: a low impact, like something internal bursting, followed by an unsettling wet silence.
**Feel:** Zero direct damage. Everything is delayed. Hemorrhage 8 is 8 damage at the start of every enemy turn — permanent. Sepsis means any Poison on top of that is doubled. The debuff finisher of the entire deck.

---

### Surgery | 3 mana | Skill | RARE *(IMPLEMENTED)*
**Effect:** Heal 20 HP.
**Art:** Top-down view of an operating table — gloved hands perfectly steady, a blinding overhead surgical lamp flooding the scene.
**AV Effect:** A soft white vignette blooms slowly from screen center, holds for 1.5 seconds, fades cleanly. The screen feels slightly warmer after. Sound: calm clinical ambience → a gentle chime when complete.
**Feel:** The lifeline. Four mana is expensive but this is the only card that significantly extends your HP pool mid-combat. In a losing fight, this buys multiple turns. A run-defining card.

---

### Radiation Blast | 3 mana | Attack | RARE
**Effect:** Deal 20 damage. Apply Brittle to the enemy (next hit deals +50% damage).
**Art:** A radioactive symbol dissolving into a forward-aimed energy wave. Clean destruction visualised.
**AV Effect:** A much brighter white flash than the existing hit effect — holds 0.3 seconds, then a slow yellow-white fade back. Screen edges crackle with visible static during the fade. Sound: building high-pitched whine → massive electrical discharge → silence.
**Feel:** 20 damage alone is among the highest direct damage in the pool. With Brittle, follow it immediately with any attack for catastrophic damage. Back-to-back with Defibrillator: 20 + 22 (15 × 1.5) for a 5-mana 2-card combo.

---

### Autopsy | 4 mana | Skill | RARE
**Effect:** Permanently remove 1 card of your choice from your deck. Draw 3 cards.
**Art:** A scalpel making a precise dissection — a ghost or silhouette rises and departs from the subject cleanly.
**AV Effect:** A quick dark fade on screen, then the image seems slightly sharper and higher contrast (subtle post-process: mild clarity filter applied). Sound: a single clean cutting sound → held silence.
**Feel:** The only non-event card that permanently thins your deck. Remove a useless card, a curse, or an overpriced common. The 3 drawn cards offset most of the mana cost in tempo. Very powerful in a deck that wants to cycle fast.

---

### Experimental Dosage | 4 mana | Gambling | RARE
**Effect:** Deal damage equal to your current HP. Reduce your HP to 1.
**Art:** An enormous syringe plunged directly into a glowing anatomical heart. Clinical brutality made beautiful.
**AV Effect:** Screen goes deep crimson. Three violent heartbeat thumps — screen shakes slightly on each one. Then goes dark and still for 0.5 seconds. Then returns. Sound: three massive heartbeat hits → a single flatline tone → a revival beep.
**Feel:** Ends fights instantly against anything weaker than you. Leaves you at 1 HP. The purest all-in in the entire game. Play it as the killing blow. Or never touch it. No middle ground.

---

### Full Recovery | 4 mana | Skill | RARE
**Effect:** Heal to your maximum HP.
**Art:** A serene hospital room at sunrise — an empty, made bed, sunlight crossing the clean floor. Rest earned.
**AV Effect:** A gentle green wave sweeps slowly from left to right across the screen. The screen warms slightly (subtle gold colour temperature shift). Sound: a long slow exhale → a soft chime.
**Feel:** Four mana is expensive mid-combat. But returning to maximum HP changes everything — the fights you win after would have been losses. Against a boss or a long encounter, a run-defining card.

---

### Neural Storm | 4 mana | Attack | RARE
**Effect:** Deal 6 damage for each unique card type in your discard pile. (Attack, Defense, Skill, Organ each count once. Maximum: 24 damage.)
**Art:** A brain at screen center — lightning radiating outward in all directions. Every neuron firing simultaneously.
**AV Effect:** Purple-white lightning crackles across the full screen in multiple arcs (random positions, overlapping). Sound: building electrical charge → large arc discharge → crackling fade.
**Feel:** Rewards deliberate variety. With all 4 types discarded: 24 damage for 4 mana. Forces you to not over-specialize. Good in diverse midrange decks, weak in pure-aggro or pure-defence builds.

---

---

# SYSTEMS & MECHANICS NOTES

### Tolerance System *(future)*
Cards tagged **[Dose]** gain +1 mana cost each time they are played during a run (not just a combat — the whole run).
Play Chemotherapy 3 times and it costs 5 mana. Encourages rotating your hand economy.
Fits the medical theme: your body adapts to and requires more of the drug over time.

### Critical Condition Passive *(future)*
When your HP drops below 30% of your maximum, all attack cards deal +50% damage.
No card required — a passive threshold rule visible in the player HUD.
Creates a dramatic floor state. The "we're in it now" moment. Rewards leaning into dangerous HP levels.

### Jackpot Visual Reference
When a Jackpot triggers (5% on any Gambling card):
- Full screen gold flash, brighter than any other effect
- A distinct audio sting (not reused from any other card)
- Card effects triple (numeric values)
- The word "JACKPOT" displayed briefly in large gold text

---

---

# NOAH'S CARDS
*Obtainable only through the Noah Event. Never appear in combat rewards or the shop.*
*All Noah cards are obscured — the player sees a garbled display name and description.*
*The real effect is only revealed when the card is played.*
*All are tagged obscured: true, hideFooterStats: true.*

Noah is not giving you medicine. He is giving you something that shouldn't exist inside the system.
His cards break rules that normal cards follow — copying, converting, reversing, preventing.
They feel irregular because they are. They are also the most powerful cards in the pool.

---

### noah_mirror | 2 mana | Skill | NOAH
**Real effect:** Play a copy of the last card you played this combat.
**Display name:** "Reflective Symptom Protocol"
**Display description:** "Mirror the most recent intervention. Results may vary."
**Art:** A scalpel reflected in a surgical mirror — the reflection slightly wrong. The angle doesn't match.
**Feel:** Doubles your best turn retroactively. Played Radiation Blast? Play it again. Played Immune Overdrive? Free turn again. Requires holding it for the right moment — useless if played first.

---

### noah_gambit | 1 mana | Attack | NOAH
**Real effect:** Lose all your current block. Deal that much damage to the enemy.
**Display name:** "Defensive Inversion Procedure"
**Display description:** "Convert excess protective surplus into targeted impact."
**Art:** A shield shattering outward — the fragments flying forward at the enemy instead of backward. Momentum redirected.
**Feel:** The riskiest conversion card. At 12 block it deals 12 damage for 1 mana. Amazing. At 0 block it does nothing. Rewards timing — play it right after a big defensive turn.

---

### noah_void_tap | 0 mana | Skill | NOAH
**Real effect:** Lose 6 HP. Draw 4 cards.
**Display name:** "Haemorrhagic Extraction Protocol"
**Display description:** "Sacrifice circulatory surplus for cognitive acceleration."
**Art:** A vein being tapped directly — dark blood drawn out, golden cards rising in its place. The exchange made literal.
**Feel:** Explosive card draw at 0 mana. The 6 HP loss is real but the 4 cards are enormous. Great in a deck that can close out fights fast. Dangerous at low HP. Synergises with Critical Condition passive — low HP, big damage.

---

### noah_ghost_marrow | 0 mana | Defense | NOAH
**Real effect:** Double your current block.
**Display name:** "Phantom Osseous Amplification"
**Display description:** "Reinforce existing structural integrity through spectral redundancy."
**Art:** A bone overlaid with a semi-transparent ghost version of itself. Two bones occupying one space. Double density.
**Feel:** Nothing at 0 block. Extraordinary at 10+ block. Play a big defensive turn, then drop this to double it. With Reinforced Bones organ: each block gain is already +3, then doubled. Scaling gets out of hand fast.

---

### noah_catalyst | 1 mana | Skill | NOAH
**Real effect:** The next card you play this turn has all numeric effects tripled.
**Display name:** "Enzymatic Cascade Initiator"
**Display description:** "Accelerate the following intervention beyond standard parameters."
**Art:** A catalyst molecule — a small compound with enormous reaction arrows branching from it. Scale implicit in the diagram.
**Feel:** Sets up a single absurd play. Catalyst → Radiation Blast = 60 damage. Catalyst → Surgery = 60 HP heal. Catalyst → Bone Marrow Surge = Draw 12. The ceiling is absurd. The skill is knowing what to triple.

---

### noah_contamination | 2 mana | Skill | NOAH
**Real effect:** Apply copies of all debuffs currently on you to the enemy as well.
**Display name:** "Bidirectional Pathogen Exchange"
**Display description:** "Redistribute active negative vectors across all present subjects."
**Art:** Two figures — one bright, one dark — connected by a tube. Whatever flows through goes both ways.
**Feel:** Useless if you have no debuffs. Devastating if an enemy has poisoned you and inflamed you. You share the misery. Against heavy debuff enemies, this is a complete reversal — they inflict it on you, you reflect it back. Pairs with enemies that stack Inflammation or Poison on the player.

---

### noah_emergency | 1 mana | Skill | NOAH
**Real effect:** If your HP is below 50% of maximum: heal 15 HP and draw 2 cards. If your HP is 50% or above: deal 12 damage.
**Display name:** "Conditional Response Matrix"
**Display description:** "Adaptive intervention scaled to current physiological state."
**Art:** A split card image — one half shows a healing IV drip, the other half shows a scalpel ready to strike. Which half glows depends on the moment.
**Feel:** Context-sensitive. Heals you when you need it. Attacks when you don't. Never a dead card. The dual nature means you never quite know which version you're holding until you play it. That moment of revelation is the fun.

---

### noah_paradox | 2 mana | Attack | NOAH
**Real effect:** Deal damage equal to the number of turns elapsed this combat × 4. Maximum 28 damage.
**Display name:** "Temporal Harm Accumulator"
**Display description:** "Leverage chronological exposure into concentrated impact."
**Art:** An hourglass with a sword embedded in it. Time and violence occupying the same object.
**Feel:** 0 damage on turn 1. 28 damage by turn 7. A card that rewards surviving a long fight and punishes playing it too early. Completely changes when you want to end fights — sometimes you stall on purpose.

---

### noah_splice | 3 mana | Skill | NOAH
**Real effect:** Reveal the top 2 cards of your deck. Play them both for free. Put nothing back.
**Display name:** "Sequential Protocol Merger"
**Display description:** "Execute the next two queued interventions simultaneously without resource expenditure."
**Art:** Two card backs merging into one object — edges overlapping, becoming a single composite. Forced together.
**Feel:** 3 mana to play 2 free cards. The value depends entirely on what's on top of your deck. With Pulse Check to set up the top 2 first: this is a controlled power spike. Blind: pure variance.

---

### noah_extraction | 2 mana | Skill | NOAH
**Real effect:** Choose and permanently remove a card from your deck. Deal damage equal to that card's mana cost × 6.
**Display name:** "Elective Excision Procedure"
**Display description:** "Permanently remove a redundant element. Convert its potential to immediate force."
**Art:** A card being pulled from a chest and dissolving — its mana cost visible and glowing as the damage it converts to.
**Feel:** Remove your worst card for free and deal 6–24 damage (0-cost card = 0 damage — read the fine print). Remove a 4-mana card: 24 damage and a thinned deck. Deck thinning plus damage in one card. Autopsy does the thinning. This does the damage.

---

### noah_override | 4 mana | Skill | NOAH
**Real effect:** Skip the enemy's next 2 turns entirely.
**Display name:** "Systemic Quarantine Extension"
**Display description:** "Extend suppression of external hostile processes by two complete cycles."
**Art:** A large red OVERRIDE stamp descending on a schedule — two rows of the enemy's timeline crossed out cleanly.
**Feel:** More expensive than Quarantine (which skips 1 turn for 4 mana) but skips 2. With Neural Bypass organ it costs 3 mana on the first play. Guarantees 2 free turns of offense or setup. Against a boss: transformative.

---

### noah_last_resort | 0 mana | Skill | NOAH
**Real effect:** *Exhaust.* Once per combat — if you would take lethal damage while this card is in your hand, you survive at 5 HP instead and this card is removed from your hand automatically.
**Display name:** "Critical Intervention Override"
**Display description:** "Emergency failsafe. Single-use. Activates automatically on terminal event."
**Art:** A defibrillator hanging on a wall behind a glass panel labelled BREAK ONLY IN EMERGENCY. The glass intact — for now.
**Feel:** A passive safety net you hold, not play. It sits in your hand doing nothing until you would die — then it fires automatically and saves you. The 0 mana cost means holding it is free. The single-use Exhaust means it's gone after. Playing recklessly knowing you have this in hand is the best feeling in the game.

---

### noah_rewrite | 3 mana | Skill | NOAH
**Real effect:** Choose any card in your hand. Transform it into a random Rare card.
**Display name:** "Diagnostic Reconfiguration Sequence"
**Display description:** "Repurpose existing intervention template into higher-order protocol."
**Art:** A card face dissolving — new symbols and colour bleeding in from the edges, forming something different. Transformation mid-frame.
**Feel:** Convert a dead card into a random Rare. Best case: Experimental Dosage or Immune Overdrive. Worst case: a Rare that doesn't fit the moment. High variance but always interesting. Great for converting a starting Bandage into something late-game relevant.

---

*Total new cards: 55 player cards + 13 Noah cards = 68 new cards designed.*
*Combined with existing 10 player + 2 Noah: 80 total cards in the game.*
