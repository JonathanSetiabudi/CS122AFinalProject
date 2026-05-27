module top (
    input  wire        clk_25m,

    output wire        lcd_clk,
    output wire        lcd_hsync,
    output wire        lcd_vsync,
    output wire        lcd_de,
    output wire [4:0]  lcd_r,
    output wire [5:0]  lcd_g,
    output wire [4:0]  lcd_b,
    output wire [4:0]  dbg
);

    wire clk_pixel;
    wire locked;
    
    pll pll_inst (
        .clkin(clk_25m),
        .clkout0(),
        .clkout1(clk_pixel),
        .locked(locked)
    );
    
    assign lcd_clk = clk_pixel;

    localparam H_ACTIVE = 480;
    localparam H_FP     = 8;
    localparam H_SYNC   = 41;
    localparam H_BP     = 43;
    localparam H_TOTAL  = 572;
    
    localparam V_ACTIVE = 272;
    localparam V_FP     = 4;
    localparam V_SYNC   = 10;
    localparam V_BP     = 12;
    localparam V_TOTAL  = 298;
    
    reg [9:0] h_cnt = 0;
    reg [8:0] v_cnt = 0;
    
    wire active = (h_cnt < H_ACTIVE) && (v_cnt < V_ACTIVE);
    
    assign lcd_hsync = ~((h_cnt >= H_ACTIVE + H_FP) && (h_cnt < H_ACTIVE + H_FP + H_SYNC));
    assign lcd_vsync = ~((v_cnt >= V_ACTIVE + V_FP) && (v_cnt < V_ACTIVE + V_FP + V_SYNC));
    assign lcd_de = active;
    
    always @(posedge clk_pixel) begin
        if (h_cnt == H_TOTAL - 1) begin
            h_cnt <= 0;
            v_cnt <= (v_cnt == V_TOTAL - 1) ? 0 : v_cnt + 1;
        end else begin
            h_cnt <= h_cnt + 1;
        end
    end

    wire [9:0] px = h_cnt;
    wire [8:0] py = v_cnt;
    
    localparam CX = 240;
    localparam CY = 210;
    localparam RADIUS = 160;
    localparam NEEDLE_LEN = 145;
    localparam RADIUS_SQ = RADIUS * RADIUS;
    
    wire signed [10:0] dx = $signed({1'b0, px}) - CX;
    wire signed [9:0]  dy = $signed({1'b0, py}) - CY;
    wire [21:0] dist2 = dx*dx + dy*dy;
    
    wire on_arc = (dist2 >= RADIUS_SQ - 300 && dist2 <= RADIUS_SQ + 300) && (dy <= 0);
    wire on_baseline = (py >= CY - 1 && py <= CY + 1);
    
    function inside;
        input [9:0] x1, x2;
        input [8:0] y1, y2;
        input [9:0] xp;
        input [8:0] yp;
        begin
            inside = (xp >= x1 && xp <= x2 && yp >= y1 && yp <= y2);
        end
    endfunction

    wire tick_0   = inside(76, 84, 207, 213, px, py);
    wire tick_15  = inside(81, 89, 166, 172, px, py);
    wire tick_30  = inside(97, 105, 127, 133, px, py);
    wire tick_45  = inside(123, 131, 94, 100, px, py);
    wire tick_60  = inside(156, 164, 68, 74, px, py);
    wire tick_75  = inside(195, 203, 53, 59, px, py);
    wire tick_90  = inside(236, 244, 47, 53, px, py);
    
    wire tick_75r = inside(277, 285, 53, 59, px, py);
    wire tick_60r = inside(316, 324, 68, 74, px, py);
    wire tick_45r = inside(349, 357, 94, 100, px, py);
    wire tick_30r = inside(375, 383, 127, 133, px, py);
    wire tick_15r = inside(391, 399, 166, 172, px, py);
    wire tick_0r  = inside(396, 404, 207, 213, px, py);
    
    wire is_tick = tick_0 || tick_15 || tick_30 || tick_45 || tick_60 || tick_75 || tick_90 ||
                   tick_75r || tick_60r || tick_45r || tick_30r || tick_15r || tick_0r;
    
    function [34:0] digit_rom;
        input [3:0] d;
        begin
            case (d)
                0: digit_rom = 35'b11111_10001_10001_10001_10001_10001_11111;
                1: digit_rom = 35'b00100_01100_00100_00100_00100_00100_01110;
                2: digit_rom = 35'b11111_00001_00001_11111_10000_10000_11111;
                3: digit_rom = 35'b11111_00001_00001_11111_00001_00001_11111;
                4: digit_rom = 35'b10001_10001_10001_11111_00001_00001_00001;
                5: digit_rom = 35'b11111_10000_10000_11111_00001_00001_11111;
                6: digit_rom = 35'b11111_10000_10000_11111_10001_10001_11111;
                7: digit_rom = 35'b11111_00001_00010_00100_01000_10000_10000;
                8: digit_rom = 35'b11111_10001_10001_11111_10001_10001_11111;
                9: digit_rom = 35'b11111_10001_10001_11111_00001_00001_11111;
                10: digit_rom = 35'b01110_10001_10000_01110_00001_10001_01110;
                11: digit_rom = 35'b11110_10001_10001_11110_10000_10000_10000;
                12: digit_rom = 35'b11110_10001_10001_10001_10001_10001_11110;
                13: digit_rom = 35'b00000_00000_01110_00000_01110_00000_00000;
                default: digit_rom = 35'b00000_00000_00000_00000_00000_00000_00000;
            endcase
        end
    endfunction
    
    function draw_digit;
        input [3:0] digit;
        input [9:0] top_x;
        input [8:0] top_y;
        input [9:0] xp;
        input [8:0] yp;
        integer col, row;
        reg [34:0] bitmap;
        begin
            draw_digit = 0;
            if (xp >= top_x && xp < top_x + 5 && yp >= top_y && yp < top_y + 7) begin
                col = xp - top_x;
                row = yp - top_y;
                bitmap = digit_rom(digit);
                draw_digit = bitmap[34 - (row*5) - col];
            end
        end
    endfunction
    
    function draw_number_spaced;
        input [3:0] tens;
        input [3:0] ones;
        input [9:0] top_x;
        input [8:0] top_y;
        input [2:0] space;
        input [9:0] xp;
        input [8:0] yp;
        begin
            draw_number_spaced = draw_digit(tens, top_x, top_y, xp, yp) ||
                                 draw_digit(ones, top_x + 5 + space, top_y, xp, yp);
        end
    endfunction
    
    wire num_L0   = draw_digit(0, 58, 218, px, py);
    wire num_L15  = draw_number_spaced(1, 5, 65, 164, 2, px, py);
    wire num_L30  = draw_number_spaced(3, 0, 83, 121, 2, px, py);
    wire num_L45  = draw_number_spaced(4, 5, 111, 85, 2, px, py);
    wire num_L60  = draw_number_spaced(6, 0, 147, 57, 2, px, py);
    wire num_L75  = draw_number_spaced(7, 5, 190, 39, 2, px, py);
    wire num_90   = draw_number_spaced(9, 0, 234, 33, 2, px, py);
    
    wire num_R75  = draw_number_spaced(7, 5, 282, 39, 2, px, py);
    wire num_R60  = draw_number_spaced(6, 0, 325, 57, 2, px, py);
    wire num_R45  = draw_number_spaced(4, 5, 361, 85, 2, px, py);
    wire num_R30  = draw_number_spaced(3, 0, 389, 121, 2, px, py);
    wire num_R15  = draw_number_spaced(1, 5, 407, 164, 2, px, py);
    wire num_R0   = draw_digit(0, 413, 218, px, py);
    
    wire left_numbers  = num_L0 || num_L15 || num_L30 || num_L45 || num_L60 || num_L75;
    wire right_numbers = num_R75 || num_R60 || num_R45 || num_R30 || num_R15 || num_R0;
    wire is_number = left_numbers || right_numbers || num_90;
    
    reg [23:0] angle_timer = 0;
    reg [7:0]  angle_deg = 0;
    reg        angle_dir = 0;
    
    always @(posedge clk_pixel) begin
        angle_timer <= angle_timer + 1;
        if (angle_timer >= 500000) begin
            angle_timer <= 0;
            if (!angle_dir) begin
                if (angle_deg >= 180) begin
                    angle_dir <= 1;
                end else begin
                    angle_deg <= angle_deg + 1;
                end
            end else begin
                if (angle_deg <= 0) begin
                    angle_dir <= 0;
                end else begin
                    angle_deg <= angle_deg - 1;
                end
            end
        end
    end

    localparam SPD_Y = 230;
    localparam SPD_START_X = 370;
    
    // Map needle angle to displayed speed (0-90-0)
    wire [7:0] display_speed;
    
    always @(*) begin
        if (angle_deg <= 90) begin
            display_speed = angle_deg;           // 0 to 90
        end else begin
            display_speed = 180 - angle_deg;     // 90 down to 0
        end
    end
    
    wire [3:0] spd_hundreds = (display_speed >= 100) ? 1 : 0;
    wire [3:0] spd_tens     = (display_speed % 100) / 10;
    wire [3:0] spd_ones     = display_speed % 10;
    
    // Draw "SPD:" label
    wire letter_S   = draw_digit(10, SPD_START_X,     SPD_Y, px, py);
    wire letter_P   = draw_digit(11, SPD_START_X + 6, SPD_Y, px, py);
    wire letter_D   = draw_digit(12, SPD_START_X + 12, SPD_Y, px, py);
    wire letter_colon = draw_digit(13, SPD_START_X + 18, SPD_Y, px, py);
    
    // Draw the speed number
    reg spd_display;
    
    always @(*) begin
        if (display_speed >= 100) begin
            spd_display = draw_digit(spd_hundreds, SPD_START_X + 30, SPD_Y, px, py) ||
                          draw_digit(spd_tens, SPD_START_X + 36, SPD_Y, px, py) ||
                          draw_digit(spd_ones, SPD_START_X + 42, SPD_Y, px, py);
        end else if (display_speed >= 10) begin
            spd_display = draw_digit(spd_tens, SPD_START_X + 30, SPD_Y, px, py) ||
                          draw_digit(spd_ones, SPD_START_X + 36, SPD_Y, px, py);
        end else begin
            spd_display = draw_digit(spd_ones, SPD_START_X + 30, SPD_Y, px, py);
        end
    end
    
    wire is_speed_display = letter_S || letter_P || letter_D || letter_colon || spd_display;

    reg signed [8:0] cos_norm;
    reg signed [8:0] sin_norm;
    
    // AI generated these numbers
    always @(*) begin
        case (angle_deg)
            0:   begin cos_norm = -128; sin_norm =   0; end
            1:   begin cos_norm = -128; sin_norm =   2; end
            2:   begin cos_norm = -128; sin_norm =   4; end
            3:   begin cos_norm = -128; sin_norm =   7; end
            4:   begin cos_norm = -128; sin_norm =   9; end
            5:   begin cos_norm = -128; sin_norm =  11; end
            6:   begin cos_norm = -127; sin_norm =  13; end
            7:   begin cos_norm = -127; sin_norm =  16; end
            8:   begin cos_norm = -127; sin_norm =  18; end
            9:   begin cos_norm = -126; sin_norm =  20; end
            10:  begin cos_norm = -126; sin_norm =  22; end
            11:  begin cos_norm = -126; sin_norm =  24; end
            12:  begin cos_norm = -125; sin_norm =  27; end
            13:  begin cos_norm = -125; sin_norm =  29; end
            14:  begin cos_norm = -124; sin_norm =  31; end
            15:  begin cos_norm = -124; sin_norm =  33; end
            16:  begin cos_norm = -123; sin_norm =  35; end
            17:  begin cos_norm = -122; sin_norm =  37; end
            18:  begin cos_norm = -122; sin_norm =  40; end
            19:  begin cos_norm = -121; sin_norm =  42; end
            20:  begin cos_norm = -120; sin_norm =  44; end
            21:  begin cos_norm = -119; sin_norm =  46; end
            22:  begin cos_norm = -119; sin_norm =  48; end
            23:  begin cos_norm = -118; sin_norm =  50; end
            24:  begin cos_norm = -117; sin_norm =  52; end
            25:  begin cos_norm = -116; sin_norm =  54; end
            26:  begin cos_norm = -115; sin_norm =  56; end
            27:  begin cos_norm = -114; sin_norm =  58; end
            28:  begin cos_norm = -113; sin_norm =  60; end
            29:  begin cos_norm = -112; sin_norm =  62; end
            30:  begin cos_norm = -111; sin_norm =  64; end
            31:  begin cos_norm = -110; sin_norm =  66; end
            32:  begin cos_norm = -109; sin_norm =  68; end
            33:  begin cos_norm = -107; sin_norm =  70; end
            34:  begin cos_norm = -106; sin_norm =  72; end
            35:  begin cos_norm = -105; sin_norm =  73; end
            36:  begin cos_norm = -104; sin_norm =  75; end
            37:  begin cos_norm = -102; sin_norm =  77; end
            38:  begin cos_norm = -101; sin_norm =  79; end
            39:  begin cos_norm = -100; sin_norm =  81; end
            40:  begin cos_norm =  -98; sin_norm =  82; end
            41:  begin cos_norm =  -97; sin_norm =  84; end
            42:  begin cos_norm =  -95; sin_norm =  86; end
            43:  begin cos_norm =  -94; sin_norm =  87; end
            44:  begin cos_norm =  -92; sin_norm =  89; end
            45:  begin cos_norm =  -91; sin_norm =  91; end
            46:  begin cos_norm =  -89; sin_norm =  92; end
            47:  begin cos_norm =  -87; sin_norm =  94; end
            48:  begin cos_norm =  -86; sin_norm =  95; end
            49:  begin cos_norm =  -84; sin_norm =  97; end
            50:  begin cos_norm =  -82; sin_norm =  98; end
            51:  begin cos_norm =  -81; sin_norm = 100; end
            52:  begin cos_norm =  -79; sin_norm = 101; end
            53:  begin cos_norm =  -77; sin_norm = 102; end
            54:  begin cos_norm =  -75; sin_norm = 104; end
            55:  begin cos_norm =  -73; sin_norm = 105; end
            56:  begin cos_norm =  -72; sin_norm = 106; end
            57:  begin cos_norm =  -70; sin_norm = 107; end
            58:  begin cos_norm =  -68; sin_norm = 109; end
            59:  begin cos_norm =  -66; sin_norm = 110; end
            60:  begin cos_norm =  -64; sin_norm = 111; end
            61:  begin cos_norm =  -62; sin_norm = 112; end
            62:  begin cos_norm =  -60; sin_norm = 113; end
            63:  begin cos_norm =  -58; sin_norm = 114; end
            64:  begin cos_norm =  -56; sin_norm = 115; end
            65:  begin cos_norm =  -54; sin_norm = 116; end
            66:  begin cos_norm =  -52; sin_norm = 117; end
            67:  begin cos_norm =  -50; sin_norm = 118; end
            68:  begin cos_norm =  -48; sin_norm = 119; end
            69:  begin cos_norm =  -46; sin_norm = 119; end
            70:  begin cos_norm =  -44; sin_norm = 120; end
            71:  begin cos_norm =  -42; sin_norm = 121; end
            72:  begin cos_norm =  -40; sin_norm = 122; end
            73:  begin cos_norm =  -37; sin_norm = 122; end
            74:  begin cos_norm =  -35; sin_norm = 123; end
            75:  begin cos_norm =  -33; sin_norm = 124; end
            76:  begin cos_norm =  -31; sin_norm = 124; end
            77:  begin cos_norm =  -29; sin_norm = 125; end
            78:  begin cos_norm =  -27; sin_norm = 125; end
            79:  begin cos_norm =  -24; sin_norm = 126; end
            80:  begin cos_norm =  -22; sin_norm = 126; end
            81:  begin cos_norm =  -20; sin_norm = 126; end
            82:  begin cos_norm =  -18; sin_norm = 127; end
            83:  begin cos_norm =  -16; sin_norm = 127; end
            84:  begin cos_norm =  -13; sin_norm = 127; end
            85:  begin cos_norm =  -11; sin_norm = 128; end
            86:  begin cos_norm =   -9; sin_norm = 128; end
            87:  begin cos_norm =   -7; sin_norm = 128; end
            88:  begin cos_norm =   -4; sin_norm = 128; end
            89:  begin cos_norm =   -2; sin_norm = 128; end
            90:  begin cos_norm =    0; sin_norm = 128; end
            91:  begin cos_norm =    2; sin_norm = 128; end
            92:  begin cos_norm =    4; sin_norm = 128; end
            93:  begin cos_norm =    7; sin_norm = 128; end
            94:  begin cos_norm =    9; sin_norm = 128; end
            95:  begin cos_norm =   11; sin_norm = 128; end
            96:  begin cos_norm =   13; sin_norm = 127; end
            97:  begin cos_norm =   16; sin_norm = 127; end
            98:  begin cos_norm =   18; sin_norm = 127; end
            99:  begin cos_norm =   20; sin_norm = 126; end
            100: begin cos_norm =   22; sin_norm = 126; end
            101: begin cos_norm =   24; sin_norm = 126; end
            102: begin cos_norm =   27; sin_norm = 125; end
            103: begin cos_norm =   29; sin_norm = 125; end
            104: begin cos_norm =   31; sin_norm = 124; end
            105: begin cos_norm =   33; sin_norm = 124; end
            106: begin cos_norm =   35; sin_norm = 123; end
            107: begin cos_norm =   37; sin_norm = 122; end
            108: begin cos_norm =   40; sin_norm = 122; end
            109: begin cos_norm =   42; sin_norm = 121; end
            110: begin cos_norm =   44; sin_norm = 120; end
            111: begin cos_norm =   46; sin_norm = 119; end
            112: begin cos_norm =   48; sin_norm = 119; end
            113: begin cos_norm =   50; sin_norm = 118; end
            114: begin cos_norm =   52; sin_norm = 117; end
            115: begin cos_norm =   54; sin_norm = 116; end
            116: begin cos_norm =   56; sin_norm = 115; end
            117: begin cos_norm =   58; sin_norm = 114; end
            118: begin cos_norm =   60; sin_norm = 113; end
            119: begin cos_norm =   62; sin_norm = 112; end
            120: begin cos_norm =   64; sin_norm = 111; end
            121: begin cos_norm =   66; sin_norm = 110; end
            122: begin cos_norm =   68; sin_norm = 109; end
            123: begin cos_norm =   70; sin_norm = 107; end
            124: begin cos_norm =   72; sin_norm = 106; end
            125: begin cos_norm =   73; sin_norm = 105; end
            126: begin cos_norm =   75; sin_norm = 104; end
            127: begin cos_norm =   77; sin_norm = 102; end
            128: begin cos_norm =   79; sin_norm = 101; end
            129: begin cos_norm =   81; sin_norm = 100; end
            130: begin cos_norm =   82; sin_norm =  98; end
            131: begin cos_norm =   84; sin_norm =  97; end
            132: begin cos_norm =   86; sin_norm =  95; end
            133: begin cos_norm =   87; sin_norm =  94; end
            134: begin cos_norm =   89; sin_norm =  92; end
            135: begin cos_norm =   91; sin_norm =  91; end
            136: begin cos_norm =   92; sin_norm =  89; end
            137: begin cos_norm =   94; sin_norm =  87; end
            138: begin cos_norm =   95; sin_norm =  86; end
            139: begin cos_norm =   97; sin_norm =  84; end
            140: begin cos_norm =   98; sin_norm =  82; end
            141: begin cos_norm =  100; sin_norm =  81; end
            142: begin cos_norm =  101; sin_norm =  79; end
            143: begin cos_norm =  102; sin_norm =  77; end
            144: begin cos_norm =  104; sin_norm =  75; end
            145: begin cos_norm =  105; sin_norm =  73; end
            146: begin cos_norm =  106; sin_norm =  72; end
            147: begin cos_norm =  107; sin_norm =  70; end
            148: begin cos_norm =  109; sin_norm =  68; end
            149: begin cos_norm =  110; sin_norm =  66; end
            150: begin cos_norm =  111; sin_norm =  64; end
            151: begin cos_norm =  112; sin_norm =  62; end
            152: begin cos_norm =  113; sin_norm =  60; end
            153: begin cos_norm =  114; sin_norm =  58; end
            154: begin cos_norm =  115; sin_norm =  56; end
            155: begin cos_norm =  116; sin_norm =  54; end
            156: begin cos_norm =  117; sin_norm =  52; end
            157: begin cos_norm =  118; sin_norm =  50; end
            158: begin cos_norm =  119; sin_norm =  48; end
            159: begin cos_norm =  119; sin_norm =  46; end
            160: begin cos_norm =  120; sin_norm =  44; end
            161: begin cos_norm =  121; sin_norm =  42; end
            162: begin cos_norm =  122; sin_norm =  40; end
            163: begin cos_norm =  122; sin_norm =  37; end
            164: begin cos_norm =  123; sin_norm =  35; end
            165: begin cos_norm =  124; sin_norm =  33; end
            166: begin cos_norm =  124; sin_norm =  31; end
            167: begin cos_norm =  125; sin_norm =  29; end
            168: begin cos_norm =  125; sin_norm =  27; end
            169: begin cos_norm =  126; sin_norm =  24; end
            170: begin cos_norm =  126; sin_norm =  22; end
            171: begin cos_norm =  126; sin_norm =  20; end
            172: begin cos_norm =  127; sin_norm =  18; end
            173: begin cos_norm =  127; sin_norm =  16; end
            174: begin cos_norm =  127; sin_norm =  13; end
            175: begin cos_norm =  128; sin_norm =  11; end
            176: begin cos_norm =  128; sin_norm =   9; end
            177: begin cos_norm =  128; sin_norm =   7; end
            178: begin cos_norm =  128; sin_norm =   4; end
            179: begin cos_norm =  128; sin_norm =   2; end
            180: begin cos_norm =  128; sin_norm =   0; end
            default: begin cos_norm = 0; sin_norm = 0; end
        endcase
    end
    
    wire signed [11:0] needle_x;
    wire signed [10:0] needle_y;
    
    assign needle_x = CX + ((cos_norm * NEEDLE_LEN) >>> 7);
    assign needle_y = CY - ((sin_norm * NEEDLE_LEN) >>> 7);
    
    wire on_needle;
    
    wire signed [10:0] line_dx = needle_x - CX;
    wire signed [9:0]  line_dy = needle_y - CY;
    wire signed [10:0] point_dx = px - CX;
    wire signed [9:0]  point_dy = py - CY;
    
    wire signed [20:0] cross = (point_dx * line_dy) - (point_dy * line_dx);
    wire [20:0] abs_cross = (cross < 0) ? -cross : cross;
    
    wire signed [20:0] dot = (point_dx * line_dx) + (point_dy * line_dy);
    wire signed [20:0] line_len_sq = (line_dx * line_dx) + (line_dy * line_dy);
    
    wire in_bbox = ((px >= CX && px <= needle_x) || (px <= CX && px >= needle_x)) &&
                   ((py >= CY && py <= needle_y) || (py <= CY && py >= needle_y));
    
    wire on_segment = (dot >= 0) && (dot <= line_len_sq);
    
    localparam THRESHOLD = 180;
    
    assign on_needle = in_bbox && on_segment && (abs_cross < THRESHOLD);

    wire white_pixel = on_arc || on_baseline || is_tick || is_number || on_needle || is_speed_display;
    
    assign lcd_r = active ? (white_pixel ? 5'b11111 : 5'b00000) : 5'b00000;
    assign lcd_g = active ? (white_pixel ? 6'b111111 : 6'b000000) : 6'b000000;
    assign lcd_b = active ? (white_pixel ? 5'b11111 : 5'b00000) : 5'b00000;
    
    assign dbg = {2'b00, lcd_hsync, lcd_vsync, lcd_de};

endmodule