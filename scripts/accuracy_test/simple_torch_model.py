import torch
import torch.nn as nn

class LeNet5(nn.Module):
    '''LeNet5 architecture in PyTorch'''
    def __init__(self, n_classes=10, in_channels=3):
        super(LeNet5, self).__init__()

        self.feature_extractor = nn.Sequential(
            nn.Conv2d(in_channels=in_channels, out_channels=6,
                      kernel_size=5, stride=1),
            nn.ReLU(inplace=True),
            nn.AvgPool2d(kernel_size=2),
            nn.Conv2d(in_channels=6, out_channels=16,
                      kernel_size=5, stride=1),
            nn.ReLU(inplace=True),
            nn.AvgPool2d(kernel_size=2),
            nn.Conv2d(in_channels=16, out_channels=120,
                      kernel_size=5, stride=1),
            nn.ReLU(inplace=True),
        )

        self.classifier = nn.Sequential(
            nn.Linear(in_features=120, out_features=n_classes),
        )

        self.name = "LeNet5"

    def forward(self, x):
        out = self.feature_extractor(x)
        out = out.view(out.size(0), -1)
        out = self.classifier(out)
        return out

def simple_torch_model_to_onnx():
    model = LeNet5(in_channels=3)
    x = torch.ones([1, 3, 32, 32], dtype=torch.float32)
    torch.onnx.export(model, x, model.name + ".onnx")

if __name__ == "__main__":
    simple_torch_model_to_onnx()