
# TECHNICAL DOCUMENTATION

1)

# STORIES


# STORY: Fade out after logo
1) After logo is loaded and faded in 0.8s, fade it out slowly, like really slowly (2s), show the opening screen, which is rendered from scenes/opening.scene.json file. The .json file defines the id: "opening_scene", layout: "grid", cols: "8", rows: 12 (both number or string) which divides the screen into 8 columns and 12 rows. and widgets: row: , col: , defines where this wigets top and left starts rendering to which row and col, and width and height units are how many cols width and how many rows of height. a bg: { image: "", color: "", "graphic": } can define the scene's background, whether an image with background color for transparent pixels or one of built-in graphic procedure with a background color, add a sample opening scene.
1.1) add some 3 bg procedures such as floating 100 triangles, floating 200 dots that get fadeIn connected lines when near by a range and fadeOut when of out of range, an effect of your choice., blurred gradients that move around the screen as orbs highly blurred and light colorful., that blend into the nearby color orbs.
1.3) Add two cards and the text "English" or "Arabic" (in arabic letters) these cards are at the bottom but one part of the rows in the grid, occupying the above two rows, and 2 columns wides spaces with its own margin of 10%. one to the left of the center line and the other to the right of the center line.
2) Play some music at the logo screen. (again procedurally generate a fade In bass sound (2s) based on a seed, if the user clicks on the logo, change this seed value and save it somewhere, indicating a change is required)
3) 
