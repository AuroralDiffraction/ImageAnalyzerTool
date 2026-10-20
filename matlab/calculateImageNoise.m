%% Noise calculation script

function img_noise = calculateImageNoise(input_img, img_name)
    if nargin < 2
        img_name = 'image';
    end

    img_std = var(double(input_img),1,3);
    img_noise = sqrt(mean(img_std,"all"));

    disp("Calculated noise of " + img_name + " [DN]: " + num2str(img_noise));
end