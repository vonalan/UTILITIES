#include "network.h"
#include "detection_layer.h"
#include "region_layer.h"
#include "cost_layer.h"
#include "utils.h"
#include "parser.h"
#include "box.h"
#include "image.h"
#include "demo.h"
#include <sys/time.h>

#define DEMO 1

#ifdef OPENCV

static char **demo_names;
static image **demo_alphabet;
static int demo_classes;

static float **probs;
static box *boxes;
static network *net;
static image buff [3];
static image buff_letter[3];
static int buff_index = 0;
static CvCapture * cap;
static IplImage  * ipl;
static float fps = 0;
static float demo_thresh = 0;
static float demo_hier = .5;
static int running = 0;

static int demo_frame = 3;
static int demo_detections = 0;
static float **predictions;
static int demo_index = 0;
static int demo_done = 0;
static float *avg;
double demo_time;

static double diff_time = 0; 
static int time_is_up = 0; 
static int update_flag = 1; 
static image *gestures;
static int cheat = 0; 
static int game_time = 1; 
static double game_start_time = 0; 
static double cur_time = 0; 
static int curound = 0; 

static int demo_indices[2]; 
static int scores[2]; 
static image demo_scores[2]; 
static image demo_gestures[2]; 
static char *demo_gesture_names[2]; 
static image demo_prompt;
static float curmprob = 0.0; 
static image cur_gesture; 

void *detect_in_thread(void *ptr)
{
    running = 1;
    float nms = .4;

    layer l = net->layers[net->n-1];
    float *X = buff_letter[(buff_index+2)%3].data;
    float *prediction = network_predict(net, X);

    memcpy(predictions[demo_index], prediction, l.outputs*sizeof(float));
    mean_arrays(predictions, demo_frame, l.outputs, avg);
    l.output = avg;
    if(l.type == DETECTION){
        get_detection_boxes(l, 1, 1, demo_thresh, probs, boxes, 0);
    } else if (l.type == REGION){
        get_region_boxes(l, buff[0].w, buff[0].h, net->w, net->h, demo_thresh, probs, boxes, 0, 0, 0, demo_hier, 1);
    } else {
        error("Last layer must produce detections\n");
    }
    if (nms > 0) do_nms_obj(boxes, probs, l.w*l.h*l.n, l.classes, nms);
    
    /* 
    printf("\033[2J");
    printf("\033[1;1H");
    printf("\nFPS:%.1f\n",fps);
    printf("Objects:\n\n");
    */

    demo_index = (demo_index + 1)%demo_frame;
    running = 0;
    return 0;
}

void *fetch_in_thread(void *ptr)
{
    int status = fill_image_from_stream(cap, buff[buff_index]);
    letterbox_image_into(buff[buff_index], net->w, net->h, buff_letter[buff_index]);
    if(status == 0) demo_done = 1;
    return 0;
}

void *display_in_thread(void *ptr)
{
    show_image_cv(buff[(buff_index + 1)%3], "Demo", ipl);
    int c = cvWaitKey(1);
    if (c != -1) c = c%256;
    if (c == 27) { // esc
        demo_done = 1;
        return 0;
    } else if (c == 82) { // R
        demo_thresh += .02;
    } else if (c == 84) { // T
        demo_thresh -= .02;
        if(demo_thresh <= .02) demo_thresh = .02;
    } else if (c == 83) {
        demo_hier += .02;
    } else if (c == 81) {
        demo_hier -= .02;
        if(demo_hier <= .0) demo_hier = .0;
    } 

    if(c == 67){ // C
        cheat = !cheat; 
    }
    if(c == 32 && game_time == 0){ // reset
        game_time = 1; 
        game_start_time = 0;
        cur_time = 0; 
        curound = 0; 
    }
    return 0;
}

void *update_prompt(void *ptr){
    // printf("diff_time: %f, diff_time: %f\n", diff_time, diff_time*10); 
    int curtime = (int)diff_time % 6; 
    time_is_up = curtime < 3 ? -1 : (curtime < 5 ? 0 : 1);

    // printf("curtime: %d, time_is_up: %d\n", curtime, time_is_up); 
    
    if(time_is_up == 0){
        update_flag = 1; 
    }
    
    if(1){
        char temp[128]; 
        sprintf(temp, "%d", 6 - curtime); 
        
        image im = buff[(buff_index + 2) %3]; 
        free_image(demo_prompt); 
        demo_prompt = get_label(demo_alphabet, temp, (im.h * 0.03)/10); 
    }
    return 0; 
}

void *shot_gesture(void *ptr)
{
    char *name;
    float mprob = 0.0; // max prob; 
    box cbox;
    int index = 0; 
        
    int i;
    for (i = 0; i < demo_detections; ++i) {
        int tmpclass = max_index(probs[i], demo_classes);
        float tmprob = probs[i][tmpclass];
        char *tmpname = demo_names[tmpclass]; 
        
        int update_flag = 0; 
        if(tmprob > demo_thresh){ 
            if(tmpclass){
                if((tmprob > mprob && index) || (tmprob < mprob && !index)){
                    update_flag = 1; 
                }
            } else {
                if(tmprob > mprob && !index){
                    update_flag = 1; 
                }
            }
        }
        
        if(update_flag){
            index = tmpclass; 
            mprob = tmprob;
            name = tmpname; 
            printf("tmpclass: %d, tmprob: %f, tmpname: %s\n", tmpclass, tmprob, tmpname); 
        
            cbox = boxes[i]; 
        }
        
        /* 
        if(tmpprob > demo_thresh && tmpprob > mprob){
            index = tmpclass; 
            mprob = tmpprob;
            name = tmpname; 
            printf("tmpclass: %d, tmprob: %f, tmpname: %s\n", tmpclass, tmpprob, tmpname); 
            
            cbox = boxes[i]; 
        }
        */ 
    }
    
    if(index){
        // printf("name*name*name: %s\n", name); 
        image im = buff[(buff_index + 2) % 3];
        
        float wtmp = cbox.w * im.w; 
        float htmp = cbox.h * im.h; 
        float tmp1 = wtmp > htmp ? wtmp : htmp; 
        
        // float tmp2 = im.h/2.0; 
        float tmp2 = im.h <= im.w ? im.h/2 : im.w/2; 
        float diam = tmp1 > tmp2 ? tmp1 : tmp2;
    
        // if(!name){left = 0, right=0, top = 0, bot = 0; }
        float wd = diam / im.w; 
        float hd = diam / im.h; 
    
        int left = (cbox.x - wd/2.0) * im.w;
        int right = (cbox.x + wd/2.0) * im.w;
        int top = (cbox.y - hd/2.0) * im.h;
        int bot = (cbox.y + hd/2.0) * im.h;
    
        int gap1 = (right - left + 1) - (im.w - 1);
        int gap2 = (bot - top + 1) - (im.h- 1); 
        int mgap = gap1 > gap2 ? gap1 : gap2; 
        if(mgap > 0){
            right -= mgap; 
            bot -= mgap; 
        }
    
        if(left < 0){
            int gap = 0 - left; 
            left += gap; 
            right += gap;
        }
        if(right > im.w - 1){
            int gap = right - (im.w - 1); 
            right -= gap; 
            left -= gap; 
        }
        if(top < 0){
            int gap = 0 - top; 
            top += gap; 
            bot += gap; 
        }
        if(bot > im.h - 1){
            int gap = bot - (im.h - 1); 
            bot -= gap; 
            top -= gap; 
        }

        if(mprob > curmprob || !cur_gesture.data){
            printf("curmprob: %f, tmpclass: %d, tmprob: %f, tmpname: %s\n", curmprob, 100, mprob, name); 
            image rtgest = crop_image(im, left, top, right - left + 1, bot - top + 1); 
            
            printf("%d, %s --> ", demo_indices[1], demo_gesture_names[1]); 
            demo_indices[1] = index; 
            demo_gesture_names[1] = name; 
            printf("%d, %s\n", demo_indices[1], demo_gesture_names[1]); 
        
            free_image(cur_gesture); 
            cur_gesture = rtgest; 
        }
    }
    /*
    else{
        demo_indices[1] = 0; 
        demo_gesture_names[1] = demo_names[0]; 
    }
    */
}

void *draw_gesture(void *ptr){
    int index = -1; 
    if(cheat){
        int idx2 = demo_indices[1]; 
        index = idx2 == 0 ? rand()%3 + 1 : (idx2 == 3 ? 1 : idx2 + 1); 
    }
    else{
        index = rand()%3 + 1; 
    }
    demo_indices[0] = index; 
    demo_gesture_names[0] = demo_names[index];
    
    free_image(demo_gestures[0]); 
    demo_gestures[0] = copy_image(gestures[index]); 
    
    return 0; 
}

void *update_gestures(void *ptr){
    printf("time is up! \n"); 
    // shot_gesture(0); 

    free_image(demo_gestures[1]); 
    demo_gestures[1] = copy_image(cur_gesture);

    draw_gesture(0); 
    
    return 0; 
}

void *update_scores(void *ptr){
    int s1 = demo_indices[0];
    int s2 = demo_indices[1]; 

    int index = -1; 
    if(s1 && s2){
        if(s1 * s2 == 3){
            index = s1 < s2 ? 0 : 1; 
        }
        else{
            index = s1 == s2 ? -1 : (s1 > s2 ? 0 : 1); 
        }
    }
    if(index > -1){
        scores[index] += 1; 
    }

    printf("round: %d, cheat: %d, index: %d, s1: %d, s2: %d, scores: %d -- %d\n\n", curound + 1, cheat, index, s1, s2, scores[0], scores[1]); 
    
    image im = buff[(buff_index + 2) % 3]; 
    
    char tmp1[128]; 
    sprintf(tmp1, "score: %d", scores[0]);
    free_image(demo_scores[0]); 
    demo_scores[0] = get_label(demo_alphabet, tmp1, (im.h*0.03)/10);  
    
    char tmp2[128]; 
    sprintf(tmp2, "score: %d", scores[1]);
    free_image(demo_scores[1]); 
    demo_scores[1] = get_label(demo_alphabet, tmp2, (im.h*0.03)/10);  
    
    return 0; 
} 

void *draw_in_thread(void *ptr)
{   
    image display = buff[(buff_index + 2) % 3]; 
    update_prompt(0); 
    
    if(game_time){
        if((1.0 - (diff_time - (int)diff_time)) < 0.5){
            // printf("time differential: %f\n", 1.0 - (diff_time - (int)diff_time));
            shot_gesture(0); 
        }
        if(time_is_up == 1 && update_flag){
            update_gestures(0);
            // draw_gesture(0); 
            update_scores(0); 
            
            update_flag = 0; 
            curmprob = 0.0;
            // free_image(cur_gesture); 
            demo_indices[1] = 0; 
            demo_gesture_names[1] = demo_names[0]; 
            
            // printf("current round: %d\n", curound + 1); 
            if(++curound == 5){
                game_time = 0; 
            }
        }
    }
    /*
    if(time_is_up == 1 && update_flag){
        update_gestures(0); 
        update_scores(0); 
        update_flag = 0; 
    }
    */
    
    // printf("\n"); 
    draw_detections(display, demo_detections, demo_thresh, boxes, probs, 0, demo_names, demo_alphabet, demo_classes);
    
    int diam = display.h <= display.w ? display.h : display.w; 
    image mim = resize_image(demo_gestures[0], diam/2, diam/2);  
    image him = resize_image(demo_gestures[1], diam/2, diam/2);  
    if(display.h <= display.w){
        embed_image(him, display, 0, display.h/2);
    } else {
        embed_image(him, display, display.w/2, 0);
    }
    embed_image(mim, display, 0, 0);  
    free_image(mim);
    free_image(him);  
    
    // float rgb[3] = {1.0, 0.0, 1.0};
    if(game_time){
        if(display.h <= display.w){
            embed_image(demo_prompt, display, (int)display.w * 0.75, 0);  
        } else {
            embed_image(demo_prompt, display, display.w - demo_prompt.w, (int)display.h * 0.75);   
        }
    }
    
    embed_image(demo_scores[0], display, 0, 0); 
    
    if(display.h <= display.w){
        embed_image(demo_scores[1], display, 0, display.h/2); 
    } else {
        embed_image(demo_scores[1], display, display.w/2, 0); 
    }
} 

void *free_my_memories(void *ptr){
    int i; 
    
    for(i = 0; i < 4; ++i){
        free_image(gestures[i]); 
    }
    
    for(i = 0; i < 2; ++i){
        free_image(demo_scores[i]); 
    }
    
    for(i = 0; i < 2; ++i){
        free_image(demo_gestures[i]); 
    }
    
    free_image(demo_prompt); 
    free_image(cur_gesture); 
}

void *display_loop(void *ptr)
{
    while(1){
        display_in_thread(0);
    }
}

void *detect_loop(void *ptr)
{
    while(1){
        detect_in_thread(0);
    }
}

void demo(char *cfgfile, char *weightfile, float thresh, int cam_index, const char *filename, char **names, int classes, int delay, char *prefix, int avg_frames, float hier, int w, int h, int frames, int fullscreen)
{
    demo_frame = avg_frames;
    predictions = calloc(demo_frame, sizeof(float*));
    image **alphabet = load_alphabet(); gestures = load_gestures(demo_names); 
    demo_names = names;
    demo_alphabet = alphabet;
    demo_classes = classes;
    demo_thresh = thresh;
    demo_hier = hier;
    printf("Demo\n");
    net = load_network(cfgfile, weightfile, 0);
    set_batch_network(net, 1);
    pthread_t detect_thread;
    pthread_t fetch_thread;

    srand(2222222);

    if(filename){
        printf("video file: %s\n", filename);
        cap = cvCaptureFromFile(filename);
    }else{
        cap = cvCaptureFromCAM(cam_index);

        if(w){
            cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_WIDTH, w);
        }
        if(h){
            cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_HEIGHT, h);
        }
        if(frames){
            cvSetCaptureProperty(cap, CV_CAP_PROP_FPS, frames);
        }
    }

    if(!cap) error("Couldn't connect to webcam.\n");

    layer l = net->layers[net->n-1];
    demo_detections = l.n*l.w*l.h;
    int j;

    avg = (float *) calloc(l.outputs, sizeof(float));
    for(j = 0; j < demo_frame; ++j) predictions[j] = (float *) calloc(l.outputs, sizeof(float));

    boxes = (box *)calloc(l.w*l.h*l.n, sizeof(box));
    probs = (float **)calloc(l.w*l.h*l.n, sizeof(float *));
    for(j = 0; j < l.w*l.h*l.n; ++j) probs[j] = (float *)calloc(l.classes+1, sizeof(float));

    buff[0] = get_image_from_stream(cap);
    buff[1] = copy_image(buff[0]);
    buff[2] = copy_image(buff[0]);
    buff_letter[0] = letterbox_image(buff[0], net->w, net->h);
    buff_letter[1] = letterbox_image(buff[0], net->w, net->h);
    buff_letter[2] = letterbox_image(buff[0], net->w, net->h);
    ipl = cvCreateImage(cvSize(buff[0].w,buff[0].h), IPL_DEPTH_8U, buff[0].c);

    int count = 0;
    if(!prefix){
        cvNamedWindow("Demo", CV_WINDOW_NORMAL); 
        if(fullscreen){
            cvSetWindowProperty("Demo", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
        } else {
            cvMoveWindow("Demo", 0, 0);
            cvResizeWindow("Demo", 1352, 1013);
        }
    }

    demo_time = what_time_is_it_now();
    game_start_time = what_time_is_it_now();
    while(!demo_done){
        // diff_time = (int)(what_time_is_it_now() - demo_time); 
        cur_time = what_time_is_it_now(); 
        diff_time = cur_time - game_start_time; 
        // printf("demo_time: %f, curtime: %f, diff_time: %f\n", demo_time, curr_time, diff_time); 
        buff_index = (buff_index + 1) %3;
        if(pthread_create(&fetch_thread, 0, fetch_in_thread, 0)) error("Thread creation failed");
        if(pthread_create(&detect_thread, 0, detect_in_thread, 0)) error("Thread creation failed");
        if(!prefix){
            fps = 1./(what_time_is_it_now() - demo_time);
            demo_time = what_time_is_it_now();
            display_in_thread(0);
        }else{
            char name[256];
            sprintf(name, "%s_%08d", prefix, count);
            save_image(buff[(buff_index + 1)%3], name);
        }
        pthread_join(fetch_thread, 0);
        pthread_join(detect_thread, 0);

        draw_in_thread(0); 

        ++count;
    }
    free_my_memories(0); 
}

void demo_compare(char *cfg1, char *weight1, char *cfg2, char *weight2, float thresh, int cam_index, const char *filename, char **names, int classes, int delay, char *prefix, int avg_frames, float hier, int w, int h, int frames, int fullscreen)
{
    demo_frame = avg_frames;
    predictions = calloc(demo_frame, sizeof(float*));
    image **alphabet = load_alphabet();
    demo_names = names;
    demo_alphabet = alphabet;
    demo_classes = classes;
    demo_thresh = thresh;
    demo_hier = hier;
    printf("Demo\n");
    net = load_network(cfg1, weight1, 0);
    set_batch_network(net, 1);
    pthread_t detect_thread;
    pthread_t fetch_thread;

    srand(2222222);

    if(filename){
        printf("video file: %s\n", filename);
        cap = cvCaptureFromFile(filename);
    }else{
        cap = cvCaptureFromCAM(cam_index);

        if(w){
            cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_WIDTH, w);
        }
        if(h){
            cvSetCaptureProperty(cap, CV_CAP_PROP_FRAME_HEIGHT, h);
        }
        if(frames){
            cvSetCaptureProperty(cap, CV_CAP_PROP_FPS, frames);
        }
    }

    if(!cap) error("Couldn't connect to webcam.\n");

    layer l = net->layers[net->n-1];
    demo_detections = l.n*l.w*l.h;
    int j;

    avg = (float *) calloc(l.outputs, sizeof(float));
    for(j = 0; j < demo_frame; ++j) predictions[j] = (float *) calloc(l.outputs, sizeof(float));

    boxes = (box *)calloc(l.w*l.h*l.n, sizeof(box));
    probs = (float **)calloc(l.w*l.h*l.n, sizeof(float *));
    for(j = 0; j < l.w*l.h*l.n; ++j) probs[j] = (float *)calloc(l.classes+1, sizeof(float));

    buff[0] = get_image_from_stream(cap);
    buff[1] = copy_image(buff[0]);
    buff[2] = copy_image(buff[0]);
    buff_letter[0] = letterbox_image(buff[0], net->w, net->h);
    buff_letter[1] = letterbox_image(buff[0], net->w, net->h);
    buff_letter[2] = letterbox_image(buff[0], net->w, net->h);
    ipl = cvCreateImage(cvSize(buff[0].w,buff[0].h), IPL_DEPTH_8U, buff[0].c);

    int count = 0;
    if(!prefix){
        cvNamedWindow("Demo", CV_WINDOW_NORMAL); 
        if(fullscreen){
            cvSetWindowProperty("Demo", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
        } else {
            cvMoveWindow("Demo", 0, 0);
            cvResizeWindow("Demo", 1352, 1013);
        }
    }

    demo_time = what_time_is_it_now();

    while(!demo_done){
        buff_index = (buff_index + 1) %3;
        if(pthread_create(&fetch_thread, 0, fetch_in_thread, 0)) error("Thread creation failed");
        if(pthread_create(&detect_thread, 0, detect_in_thread, 0)) error("Thread creation failed");
        if(!prefix){
            fps = 1./(what_time_is_it_now() - demo_time);
            demo_time = what_time_is_it_now();
            display_in_thread(0);
        }else{
            char name[256];
            sprintf(name, "%s_%08d", prefix, count);
            save_image(buff[(buff_index + 1)%3], name);
        }
        pthread_join(fetch_thread, 0);
        pthread_join(detect_thread, 0);
        ++count;
    }
}
#else
void demo(char *cfgfile, char *weightfile, float thresh, int cam_index, const char *filename, char **names, int classes, int delay, char *prefix, int avg, float hier, int w, int h, int frames, int fullscreen)
{
    fprintf(stderr, "Demo needs OpenCV for webcam images.\n");
}
#endif

