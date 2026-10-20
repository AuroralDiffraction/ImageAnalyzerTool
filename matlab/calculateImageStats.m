%% Noise calculation script

function [img_mean, img_std] = calculateImageStats(input_img, label)
    if nargin < 2
        label = 'image';  
    end

    img_std = std(double(input_img),1,"all");
    img_mean = mean(double(input_img),"all");

    disp("Calculated mean of "+label+" in DN: "+ num2str(img_mean));
    disp("Calculated std of "+label+" in DN: "+ num2str(img_std));
end