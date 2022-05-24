// ITK includes
#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkLabelMap.h>
#include <itkLabelImageToLabelMapFilter.h>
#include <itkLabelMapToLabelImageFilter.h>
#include <itkLabelSelectionLabelMapFilter.h>
#include <itkBinaryImageToShapeLabelMapFilter.h>
#include <itkMergeLabelMapFilter.h>
#include <itkChangeLabelImageFilter.h>

// TCLAP includes
#include <tclap/ValueArg.h>
#include <tclap/MultiArg.h>
#include <tclap/ArgException.h>
#include <tclap/CmdLine.h>

// STD includes
#include <cstdlib>

// NOTE: For now we will assume images to compare are float and mask is unsigned short

int main (int argc, char **argv)
{

  // =========================================================================
  // Command-line variables
  // =========================================================================
  std::string inputFileName;
  std::string outputFileName;
  std::vector<unsigned short> labels;
  unsigned short outputLabel;

  // =========================================================================
  // Parse arguments
  // =========================================================================
  try {

    TCLAP::CmdLine cmd("itkLabelExtract");

    TCLAP::ValueArg<std::string> imageInput("i", "input", "Input Image", true, "None", "string");
    TCLAP::ValueArg<std::string> imageOutput("o", "output", "Output Image", true, "None", "string");
    TCLAP::MultiArg<unsigned short> labelsInput("l", "label", "Label to extract", true, "unsigne short");
    TCLAP::ValueArg<unsigned short> labelOutput("L", "output_label", "Label for the output image", false, 1, "unsigned short");

    cmd.add(imageInput);
    cmd.add(imageOutput);
    cmd.add(labelsInput);
    cmd.add(labelOutput);

    cmd.parse(argc,argv);

    inputFileName = imageInput.getValue();
    outputFileName = imageOutput.getValue();
    labels = labelsInput.getValue();
    outputLabel = labelOutput.getValue();

  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
  }

  // =========================================================================
  // ITK definitions
  // =========================================================================
  using PixelType = unsigned short;
  using ImageType = itk::Image<PixelType, 3>;
  using ImageReaderType = itk::ImageFileReader<ImageType>;
  using ImageWriterType = itk::ImageFileWriter<ImageType>;
  using BI2SLMType = itk::BinaryImageToShapeLabelMapFilter<ImageType>;
  using LabelMapType = BI2SLMType::OutputImageType;
  using LabelObjectType = BI2SLMType::LabelObjectType;
  using MergerType = itk::MergeLabelMapFilter<LabelMapType>;
  using LabelMapToLabelImageFilterType = itk::LabelMapToLabelImageFilter<LabelMapType, ImageType>;
  using LabelImageToLabelMapFilterType = itk::LabelImageToLabelMapFilter<ImageType, LabelMapType>;
  using SelectorType = itk::LabelSelectionLabelMapFilter<LabelMapType>;
  using ChangeLabelmapFilterType = itk::ChangeLabelImageFilter<ImageType, ImageType>;

  // =========================================================================
  // Image loading and checking
  // =========================================================================
  auto imageReader= ImageReaderType::New();
  imageReader->SetFileName(inputFileName);
  imageReader->Update();

  auto labelMapConverter = LabelImageToLabelMapFilterType::New();
  labelMapConverter->SetInput(imageReader->GetOutput());
  labelMapConverter->SetBackgroundValue(itk::NumericTraits<PixelType>::Zero);

  // =========================================================================
  // Image loading and checking
  // =========================================================================
  auto merger = MergerType::New();
  merger->SetMethod(itk::MergeLabelMapFilterEnums::ChoiceMethod::PACK);

  for (int i = 0; i<labels.size(); ++i )
    {
    auto selector = SelectorType::New();
    selector->SetInput(labelMapConverter->GetOutput());
    selector->SetLabel(labels[i]);
    selector->Update();

    merger->SetInput(i, selector->GetOutput());
  }

  merger->Update();

  auto labelImageConverter = LabelMapToLabelImageFilterType::New();
  labelImageConverter->SetInput(merger->GetOutput());
  labelImageConverter->Update();

  // =========================================================================
  // Set the new label
  // =========================================================================
  auto changeLabelmapFilter = ChangeLabelmapFilterType::New();
  changeLabelmapFilter->SetInput(labelImageConverter->GetOutput());

  for (auto label: labels)
    {
    changeLabelmapFilter->SetChange(label, outputLabel);
    }

  changeLabelmapFilter->Update();

  // =========================================================================
  // Write out the masked images (optional)
  // =========================================================================
  try {
    itk::WriteImage(changeLabelmapFilter->GetOutput(), outputFileName);
  } catch (const itk::ExceptionObject &error) {
    std::cerr << "Error: " << error << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
